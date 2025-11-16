#include <LibDegorasSLR/ILRS/algorithms/statistics.h>
#include <LibDegorasBase/Helpers/container_helpers.h>
#include <LibDegorasBase/Statistics/fitting.h>
#include <LibDegorasBase/Statistics/histogram.h>


namespace algorithm{

template <typename T>
std::vector<std::size_t> windowPrefilterPrivate(const std::vector<T> &resids, T upper, T lower)
{
    // Check the input.
    if(resids.empty() || upper <= lower)
        return {};

    // Auxiliar containers.
    std::vector<std::size_t> indexes;

    // Get the acepted residuals.
    for(std::size_t i = 0; i < resids.size(); i++)
        if(resids[i] <= upper && resids[i] >= lower)
            indexes.push_back(i);

    // Return the indexes.
    return indexes;
}

std::vector<std::size_t> histPrefilterBinSLR(const std::vector<double> &resids_bin, double depth, unsigned min_ph);

/**
 * @brief Custom count bin function.
 *
 * This function counts how many values in the container are in the given interval. The boundaries can be
 * customized as open or closed intervals (in the mathematical sense). The default interval is [min, max).
 *
 * @param[in] container The container with the values.
 * @param[in] min
 * @param[in] max
 * @param[in] ex_min True if you want to exclude the minimum value (open interval).
 * @param[in] ex_max True if you want to exclude the maximum value (open interval).
 * @return The number of elements in the container that are in the given interval.
 * @warning When comparing floating-point values, precision issues may arise due to the inherent limitations
 *          of floating-point representation. Take care when comparing floating-point values.
 */
template <typename Container, typename T>
unsigned countBin(const Container& container, T min,T max, bool exmin = false, bool exmax = true)
{
    // Convenient alias.
    using ConType = typename Container::value_type;

    // Count the values.
    unsigned counter = std::count_if(container.begin(), container.end(),
                                     [&min, &max, &exmin, &exmax](const ConType& i)
                                     {
                                         bool result;
                                         if(exmin && exmax)
                                             result = (i > min) && (i < max);
                                         else if(exmin && !exmax)
                                             result = (i > min) && (i <= max);

                                         else if(!exmin && exmax)
                                             result = (i >= min) && (i < max);
                                         else
                                             result = (i >= min) && (i <= max);
                                         return result;
                                     });
    // Return the result.
    return counter;
}


template <typename C>
dpbase::stats::types::HistCountRes<C> histcounts1D(const C& data, size_t nbins, typename C::value_type min_edge,
                                                   typename C::value_type max_edge)
{
    // Convenient alias.
    using ConType = typename C::value_type;

    // Return container.
    std::vector<std::tuple<unsigned, ConType, ConType>> result(nbins);

    // Get the division.
    ConType div = (max_edge - min_edge) / nbins;

    // Parallel loop for each bin.
    for (size_t i = 0; i < nbins; i++ )
    {
        // Update the next counter.
        ConType min = min_edge + i * div;
        ConType max = min + div;
        // Count the data in the bin.
        unsigned counter = countBin(data, min,  max);
        // Push the new data in the result vector, and update the min counter.
        result[i] = {counter, min, max};
    }

    // Return the result.
    return result;
}

template <typename C>
dpbase::stats::types::HistCountBin<C> histcounts1D(const C& data, size_t nbins)
{
    // Convenient alias.
    using ConType = typename C::value_type;

    // Return container.
    std::vector<std::tuple<unsigned, ConType, ConType>> result(nbins);

    // Get the minimum and maximum values.
    auto minmax = std::minmax_element(data.begin(), data.end());
    ConType min_counter = *(minmax.first);
    ConType max_counter = *(minmax.second);

    // Get the division.
    ConType div = (std::abs(max_counter) + std::abs(min_counter)) / nbins;

    // Parallel loop for each bin.
    omp_set_num_threads(omp_get_num_procs());
#pragma omp parallel for
    for (size_t i = 0; i < nbins; i++ )
    {
        // Update the next counter.
        ConType min = min_counter + i * div;
        ConType max = min + div;
        // Count the data in the bin.
        unsigned counter = countBin(data, min,  max);
        // Push the new data in the result vector, and update the min counter.
        result[i] = {counter, min, max};
    }

    // Return the result.
    return result;
}


template<typename T, typename R>
std::vector<std::vector<std::size_t>> extractBins(const std::vector<T> &times, const std::vector<R> &resids,
                                                             double bs)
{
    // Check the input data.
    if (times.empty() || resids.empty() || times.size() != resids.size() || bs <= 0)
        return {};

    // Containers and auxiliar variables.
    std::vector<std::vector<std::size_t>> bins;
    std::vector<std::size_t> current_bin;


    // Get the first bin.
    int last_bin = static_cast<int>(std::floor(times[0]/bs) + 1);

    // Generate the bins.
    for (std::size_t i = 0; i < times.size(); i++)
    {
        // Get the current bin.
        int bin = static_cast<int>(std::floor(times[i]/bs) + 1);

        // Check if the current bin has changed.
        if(last_bin != bin)
        {
            // New bin section.
            last_bin = bin;
            bins.push_back(std::move(current_bin));

            // Clear the bin container.
            current_bin = {};
        }

        // Stores the bin data.
        current_bin.push_back(i);
    }

    // Store the last bin.
    bins.push_back(std::move(current_bin));


    // Return the bins.
    return bins;
}


std::vector<std::size_t> histPrefilterSLR(const std::vector<double> &times, const std::vector<double> &resids,
                                                     double bs, double depth, unsigned min_ph, unsigned divisions)
{
    // Check the input data.
    if (times.empty() || resids.empty() || times.size() != resids.size() || depth <= 0 || bs <= 0 || divisions <= 0)
        return {};

    // Containers and auxiliar variables.
    std::vector<std::size_t> selected_ranges;
    std::size_t first = 0;
    double _depth = depth/divisions;
    unsigned _min_ph = min_ph/divisions;
    auto bins_indexes = algorithm::extractBins(times, resids, bs);

    for (const auto& bin_indexes : bins_indexes)
    {
        // Compute selected ranges from the bin
        auto bin_resids = dpbase::helpers::containers::extract(resids, bin_indexes);
        auto bin_selected = algorithm::histPrefilterBinSLR(bin_resids, _depth, _min_ph);
        std::transform(bin_selected.begin(), bin_selected.end(), std::back_inserter(selected_ranges),
                       [first](const auto& idx){return  idx + first;});

        // Update the first bin index variable.
        first += bin_indexes.size();
    }

    // Return the result container.
    return selected_ranges;
}

std::vector<std::size_t> histPrefilterBinSLR(const std::vector<double> &resids_bin, double depth, unsigned min_ph)
{
    std::vector<std::size_t> selected_ranges;

    // Check if the residuals bin is not empty.
    if(!resids_bin.empty())
    {
        // Compute the range gate width.
        auto edges = std::minmax_element(resids_bin.begin(), resids_bin.end());
        long double rg_width = std::abs(*edges.first) + std::abs(*edges.second);

        // Get the histogram division size.
        std::size_t hist_size = static_cast<std::size_t>(std::floor(rg_width/depth));

        // Calculate histogram of residuals in bin.
        auto histcount_res = histcounts1D(resids_bin, hist_size, *edges.first, *edges.second);


        // Check which histogram bins have more than min_photons count. They will be selected.
        //        std::vector<std::size_t> sel_bins;
        //        for (std::size_t bin_idx = 0; bin_idx < histcount_res.size(); bin_idx++)
        //            if (std::get<0>(histcount_res[bin_idx]) >= min_photons)
        //                sel_bins.push_back(bin_idx);

        //        // Get points which are inside of selected histogram bins
        //        for (std::size_t res_idx = 0; res_idx < resids_bin.size(); res_idx++)
        //        {
        //            bool select = false;
        //            std::size_t sel_bin_idx = 0;
        //            while(!select && sel_bin_idx < sel_bins.size())
        //            {
        //                const auto& bin_res = histcount_res[sel_bins[sel_bin_idx]];
        //                if (resids_bin[res_idx] >= std::get<1>(bin_res) && resids_bin[res_idx] < std::get<2>(bin_res))
        //                    select = true;
        //                sel_bin_idx++;
        //            }

        //            // Store the selected range.
        //            if (select)
        //                selected_ranges.push_back(res_idx);
        //        }

        //        auto it = std::max_element(histcount_res.begin(), histcount_res.end(),
        //                         [](const auto& a, const auto& b){return std::get<0>(a) < std::get<0>(b);});

        //        if (it != histcount_res.end() && std::get<0>(*it) >= min_photons)
        //        {
        //            // Get points which are inside of selected histogram bins
        //            for (std::size_t res_idx = 0; res_idx < resids_bin.size(); res_idx++)
        //            {
        //                if (resids_bin[res_idx] >= std::get<1>(*it) && resids_bin[res_idx] < std::get<2>(*it))
        //                    selected_ranges.push_back(res_idx);
        //            }
        //        }

        auto it = std::max_element(histcount_res.begin(), histcount_res.end(),
                                   [](const auto& a, const auto& b){return std::get<0>(a) < std::get<0>(b);});

        if (it != histcount_res.end() && std::get<0>(*it) >= min_ph)
        {
            // Iterator pointing to first bin NOT valid
            auto it_lower = it - 1;
            auto it_upper = it + 1;

            while (it_lower >= histcount_res.begin() && std::get<0>(*it_lower) >= min_ph)
                it_lower--;

            while (it_upper != histcount_res.end() && std::get<0>(*it_upper) >= min_ph)
                it_upper++;

            // Get points which are inside of selected histogram bins
            for (std::size_t res_idx = 0; res_idx < resids_bin.size(); res_idx++)
            {
                bool select = false;
                auto it_bin = it_lower + 1;
                while(!select && it_bin < it_upper)
                {
                    if (resids_bin[res_idx] >= std::get<1>(*it_bin) && resids_bin[res_idx] < std::get<2>(*it_bin))
                        select = true;
                    it_bin++;
                }

                // Store the selected range.
                if (select)
                    selected_ranges.push_back(res_idx);
            }
        }

    }

    return selected_ranges;
}

std::vector<std::size_t> histPostfilterSLR(const std::vector<double> &times, const std::vector<double> &data,
                                           double bs, double depth)
{
    // Call to the prefilter disabling the ph contributions.
    //return histPrefilterSLR(times, data, bs, depth, 0);

    // Auxiliar containers.
    std::vector<double> detrend_resids;
    std::vector<std::size_t> sel_indexes;
    double rf = depth *1.5; // depth / 2 * 2.5
    std::vector<double> y_vec;

    // Detrend the residuals.
    //detrend_resids = dpslr::math::detrend(times, data, 9);

    auto coefs = dpbase::stats::polynomialFit(times, data, 9);

    for (std::size_t i = 0; i < data.size(); i++)
    {
        double y_interp = dpbase::stats::applyPolynomial(coefs, times[i]);

        if (data[i] >= y_interp - rf && data[i] <= y_interp + rf)
            sel_indexes.push_back(i);

    }
    return sel_indexes;
}
}
