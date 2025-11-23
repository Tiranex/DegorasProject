#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include <mongocxx/instance.hpp>
#include <iostream>

int main(int argc, char *argv[])
{

    // Crear la instancia de MONGOCXX ANTES de cualquier cosa
    mongocxx::instance instance{};


    // Crear la aplicación Qt
    QApplication a(argc, argv);
    // Set dark theme style
    a.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    // Apply palette to the application
    a.setPalette(darkPalette);

    // Crear y mostrar nuestra ventana principal
    // (El constructor de MainWindow se conectará a la BBDD)
    try {
        MainWindow w;
        w.show();
        // Iniciar el buloop de eventos de la aplicación
        return a.exec();
    } catch (const std::exception& e) {
        // Si el constructor de MainWindow (p.ej. la conexión a Mongo) falla,
        // lo mostramos.
        std::cerr << "Error fatal al iniciar: " << e.what() << std::endl;
        // Podríamos usar un QMessageBox aquí, pero std::cerr es más simple
        // si la GUI ni siquiera puede arrancar.
        return 1;
    }
}
