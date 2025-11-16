#include "mainwindow.h"

#include <QApplication>
#include <mongocxx/instance.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
    // Crear la instancia de MONGOCXX ANTES de cualquier cosa
    mongocxx::instance instance{};

    // Crear la aplicación Qt
    QApplication a(argc, argv);

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
