#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
using namespace std;

// Da formato a cantidades enteras en pesos chilenos: 14990 -> $14.990.
inline string formatoPesos(int monto) {
    string s = to_string(monto);
    string r = "";
    int cont = 0;
    for (int i = (int)s.size() - 1; i >= 0; i--) {
        r = s[i] + r;
        cont++;
        if (cont % 3 == 0 && i > 0) r = "." + r;
    }
    return "$" + r;
}

inline string leerTexto(const string& mensaje) {
    string t;
    cout << mensaje;
    if (!getline(cin, t)) exit(0);
    return t;
}

// Recibe un entero del usuario y evita entradas fuera del rango permitido.
inline int leerEntero(const string& mensaje, int minimo, int maximo) {
    while (true) {
        string t = leerTexto(mensaje);
        bool valido = !t.empty() && t.size() <= 6;
        for (size_t i = 0; i < t.size(); i++)
            if (t[i] < '0' || t[i] > '9') valido = false;
        if (valido) {
            int n = atoi(t.c_str());
            if (n >= minimo && n <= maximo) return n;
        }
        cout << "Ingrese un numero entre " << minimo << " y " << maximo << "." << endl;
    }
}

// Cada nodo representa una linea del pedido, no existencias de bodega.
// El puntero siguiente enlaza el producto con el proximo nodo.
struct NodoLista {
    string producto;
    int gramos;
    int precioKilo;
    NodoLista* siguiente;
};

// Permite agregar, buscar y quitar productos del pedido en curso.
class ListaEnlazada {
private:
    NodoLista* inicio;

    static int subtotal(const NodoLista* n) {
        return n->precioKilo * n->gramos / 1000;
    }

public:

    ListaEnlazada() : inicio(nullptr) {}

    ~ListaEnlazada() { vaciar(); }

    ListaEnlazada(const ListaEnlazada&) = delete;
    ListaEnlazada& operator=(const ListaEnlazada&) = delete;

    bool estaVacia() const { return inicio == nullptr; }

    void vaciar() {
        while (inicio != nullptr) {
            NodoLista* aux = inicio;
            inicio = inicio->siguiente;
            delete aux;
        }
    }

    // Inserta al comienzo del pedido; no altera los turnos de clientes.
    void insertarInicio(const string& producto, int gramos, int precioKilo) {
        NodoLista* nuevo = new NodoLista{producto, gramos, precioKilo, inicio};
        inicio = nuevo;
    }

    // Recorre la lista y agrega un nodo al final.
    void insertarFinal(const string& producto, int gramos, int precioKilo) {
        NodoLista* nuevo = new NodoLista{producto, gramos, precioKilo, nullptr};
        if (inicio == nullptr) {
            inicio = nuevo;
            return;
        }
        NodoLista* actual = inicio;
        while (actual->siguiente != nullptr)
            actual = actual->siguiente;
        actual->siguiente = nuevo;
    }

    // Elimina la PRIMERA coincidencia del nombre del producto.
    bool eliminar(const string& producto) {
        NodoLista* actual = inicio;
        NodoLista* anterior = nullptr;
        while (actual != nullptr && actual->producto != producto) {
            anterior = actual;
            actual = actual->siguiente;
        }
        if (actual == nullptr) return false;
        if (anterior == nullptr)
            inicio = actual->siguiente;
        else
            anterior->siguiente = actual->siguiente;
        delete actual;
        return true;
    }

    const NodoLista* buscar(const string& producto) const {
        const NodoLista* actual = inicio;
        while (actual != nullptr) {
            if (actual->producto == producto) return actual;
            actual = actual->siguiente;
        }
        return nullptr;
    }

    int total() const {
        int suma = 0;
        const NodoLista* actual = inicio;
        while (actual != nullptr) {
            suma += subtotal(actual);
            actual = actual->siguiente;
        }
        return suma;
    }

    void mostrar() const {
        if (inicio == nullptr) {
            cout << "  (pedido vacio)" << endl;
            return;
        }
        const NodoLista* actual = inicio;
        int pos = 1;
        while (actual != nullptr) {
            cout << "  " << pos++ << ". " << left << setw(16) << actual->producto
                 << right << setw(6) << actual->gramos << " g  x "
                 << formatoPesos(actual->precioKilo) << "/kg = "
                 << formatoPesos(subtotal(actual)) << endl;
            actual = actual->siguiente;
        }
        cout << "  TOTAL: " << formatoPesos(total()) << endl;
    }
};
// Resumen de una venta. No se almacena el detalle del pedido en la boleta.
struct Boleta {
    int numero;
    string cliente;
    int total;
};

struct NodoPila {
    Boleta boleta;
    NodoPila* abajo;
};

// Historial de boletas en orden LIFO (ultima emitida, primera retirada).
class Pila {
private:
    NodoPila* tope;

public:
    Pila() : tope(nullptr) {}

    ~Pila() {
        while (tope != nullptr) {
            NodoPila* aux = tope;
            tope = tope->abajo;
            delete aux;
        }
    }

    Pila(const Pila&) = delete;
    Pila& operator=(const Pila&) = delete;

    bool estaVacia() const { return tope == nullptr; }

    // PUSH coloca una boleta nueva en el tope.
    void push(const Boleta& b) {
        NodoPila* nuevo = new NodoPila{b, tope};
        tope = nuevo;
    }

    // POP retira solamente la boleta que esta en el tope.
    bool pop(Boleta& b) {
        if (estaVacia()) return false;
        NodoPila* aux = tope;
        b = aux->boleta;
        tope = tope->abajo;
        delete aux;
        return true;
    }

    int totalVendido() const {
        int suma = 0;
        const NodoPila* actual = tope;
        while (actual != nullptr) {
            suma += actual->boleta.total;
            actual = actual->abajo;
        }
        return suma;
    }

    void mostrar() const {
        if (estaVacia()) {
            cout << "  (no hay boletas)" << endl;
            return;
        }
        const NodoPila* actual = tope;
        while (actual != nullptr) {
            cout << "  | Boleta Nro. " << actual->boleta.numero << " - "
                 << actual->boleta.cliente << " - "
                 << formatoPesos(actual->boleta.total) << endl;
            actual = actual->abajo;
        }
    }
};

// Nodo de la fila de atencion: cada nodo guarda el turno y nombre.
struct NodoCola {
    string dato;
    NodoCola* siguiente;
};

// Cola FIFO: se agrega al final y se atiende desde el frente.
class Cola {
private:
    NodoCola* frente;
    NodoCola* final;

public:
    Cola() : frente(nullptr), final(nullptr) {}

    ~Cola() {
        while (frente != nullptr) {
            NodoCola* aux = frente;
            frente = frente->siguiente;
            delete aux;
        }
    }

    Cola(const Cola&) = delete;
    Cola& operator=(const Cola&) = delete;

    bool estaVacia() const { return frente == nullptr; }

    int cantidad() const {
        int n = 0;
        const NodoCola* actual = frente;
        while (actual != nullptr) {
            n++;
            actual = actual->siguiente;
        }
        return n;
    }

    // ENQUEUE: agrega un cliente al final de la fila.
    void InsertarQ(const string& valor) {
        NodoCola* nuevo = new NodoCola{valor, nullptr};
        if (estaVacia()) {
            frente = nuevo;
            final = nuevo;
        } else {
            final->siguiente = nuevo;
            final = nuevo;
        }
    }

    // DEQUEUE: saca al cliente con mayor tiempo de espera.
    bool EliminarQ(string& valor) {
        if (estaVacia()) return false;
        NodoCola* aux = frente;
        valor = aux->dato;
        frente = frente->siguiente;
        if (frente == nullptr)
            final = nullptr;
        delete aux;
        return true;
    }

    void mostrar() const {
        if (estaVacia()) {
            cout << "  (no hay clientes en espera)" << endl;
            return;
        }
        const NodoCola* actual = frente;
        while (actual != nullptr) {
            cout << "  -> " << actual->dato << endl;
            actual = actual->siguiente;
        }
    }
};
// Catalogo fijo de ejemplo; no controla kilos disponibles en bodega.
const int NPROD = 8;
const string CATALOGO[NPROD] = {"Lomo vetado", "Posta", "Plateada", "Asado de tira",
                                "Costillar", "Longaniza", "Pollo entero", "Molida especial"};
const int PRECIOS[NPROD] = {14990, 9990, 10490, 11990, 8990, 6990, 3990, 7490};

int elegirProducto() {
    cout << "  Productos disponibles:" << endl;
    for (int i = 0; i < NPROD; i++)
        cout << "   " << i + 1 << ". " << CATALOGO[i] << " (" << formatoPesos(PRECIOS[i]) << "/kg)" << endl;
    return leerEntero("  Producto (0 = cancelar): ", 0, NPROD) - 1;
}

// Submenu del pedido de un cliente atendido.
void menuPedido(ListaEnlazada& pedido, const string& cliente) {
    int op = -1;
    while (op != 0) {
        cout << "\n--- PEDIDO DE " << cliente << " [LISTA ENLAZADA] ---\n"
             << "1. Agregar producto al final       (insertarFinal)\n"
             << "2. Agregar producto URGENTE (primero)  (insertarInicio)\n"
             << "3. Quitar un producto              (eliminar)\n"
             << "4. Buscar un producto en el pedido (buscar)\n"
             << "5. Ver pedido y total\n"
             << "0. Volver\n";
        op = leerEntero("Opcion: ", 0, 5);
        if (op == 1 || op == 2) {
            int p = elegirProducto();
            if (p < 0) continue;
            int g = leerEntero("  Cantidad en gramos (100 a 20000): ", 100, 20000);
            if (op == 1) pedido.insertarFinal(CATALOGO[p], g, PRECIOS[p]);
            else         pedido.insertarInicio(CATALOGO[p], g, PRECIOS[p]);
            cout << "Agregado: " << CATALOGO[p] << " " << g << " g"
                 << (op == 2 ? " (al inicio del pedido)" : "") << endl;
        } else if (op == 3) {
            if (pedido.estaVacia()) { cout << "El pedido esta vacio." << endl; continue; }
            int p = elegirProducto();
            if (p < 0) continue;
            if (pedido.eliminar(CATALOGO[p])) cout << "Se quito " << CATALOGO[p] << " del pedido." << endl;
            else cout << CATALOGO[p] << " no esta en el pedido." << endl;
        } else if (op == 4) {
            int p = elegirProducto();
            if (p < 0) continue;
            const NodoLista* n = pedido.buscar(CATALOGO[p]);
            if (n != nullptr) cout << "Si: " << n->producto << ", " << n->gramos << " g en el pedido." << endl;
            else cout << CATALOGO[p] << " no esta en el pedido." << endl;
        }
        if (op >= 1 && op <= 5) {
            cout << "Pedido actual:" << endl;
            pedido.mostrar();
        }
    }
}

// Flujo: tomar turno -> atender -> agregar productos -> cobrar.
int main() {
    Cola fila;
    ListaEnlazada pedido;
    Pila boletas;
    string clienteActual;
    bool hayCliente = false;
    int contadorTurno = 0;
    int numBoleta = 0;
    int op = -1;

    while (op != 0) {
        cout << "\n========== CARNICERIA EL BUEN CORTE ==========\n"
             << "Cliente en atencion: " << (hayCliente ? clienteActual : "ninguno")
             << " | En espera: " << fila.cantidad() << "\n"
             << "1. Tomar turno (llega un cliente)          [COLA: InsertarQ]\n"
             << "2. Ver fila de espera                      [COLA]\n"
             << "3. Atender al siguiente cliente            [COLA: EliminarQ]\n"
             << "4. Armar el pedido del cliente atendido    [LISTA ENLAZADA]\n"
             << "5. Cobrar y emitir boleta                  [PILA: push]\n"
             << "6. Cancelar pedido (el cliente se retira)  [LISTA: vaciar]\n"
             << "7. Anular la ultima boleta                 [PILA: pop]\n"
             << "8. Ver boletas del dia y total vendido     [PILA]\n"
             << "9. Cargar datos de ejemplo\n"
             << "0. Cerrar el local\n";
        op = leerEntero("Opcion: ", 0, 9);

        if (op == 1) {
            string nombre = leerTexto("Nombre del cliente: ");
            if (nombre.empty()) {
                cout << "Debe ingresar un nombre." << endl;
            } else {
                contadorTurno++;
                fila.InsertarQ("Turno " + to_string(contadorTurno) + ": " + nombre);
                cout << "Ticket entregado: Turno " << contadorTurno << " (" << nombre << "). "
                     << "Clientes en espera: " << fila.cantidad() << endl;
            }
        } else if (op == 2) {
            cout << "Fila de espera (el primero es el proximo en ser atendido):" << endl;
            fila.mostrar();
        } else if (op == 3) {
            if (hayCliente) {
                cout << "Aun esta atendiendo a " << clienteActual
                     << ". Cobre o cancele el pedido primero." << endl;
            } else if (fila.EliminarQ(clienteActual)) {
                hayCliente = true;
                cout << "Atendiendo a: " << clienteActual << endl;
            } else {
                cout << "No hay clientes en espera." << endl;
            }
        } else if (op == 4) {
            if (!hayCliente) cout << "No hay cliente en atencion. Use la opcion 3." << endl;
            else menuPedido(pedido, clienteActual);
        } else if (op == 5) {
            if (!hayCliente) {
                cout << "No hay cliente en atencion." << endl;
            } else if (pedido.estaVacia()) {
                cout << "El pedido esta vacio. Agregue productos o cancele el pedido." << endl;
            } else {
                Boleta b = {++numBoleta, clienteActual, pedido.total()};
                cout << "\n----- BOLETA Nro. " << b.numero << " -----\n"
                     << "Cliente: " << b.cliente << endl;
                pedido.mostrar();
                boletas.push(b);
                pedido.vaciar();
                hayCliente = false;
                cout << "Boleta emitida. Gracias por su compra." << endl;
            }
        } else if (op == 6) {
            if (!hayCliente) {
                cout << "No hay cliente en atencion." << endl;
            } else {
                pedido.vaciar();
                cout << clienteActual << " se retira sin comprar. Pedido cancelado." << endl;
                hayCliente = false;
            }
        } else if (op == 7) {
            // Anulacion demostrativa: quita el ultimo REGISTRO de la pila.
            // No devuelve dinero, no guarda auditoria y no restaura stock.
            Boleta b;
            if (boletas.pop(b))
                cout << "Boleta anulada: Nro. " << b.numero << " - " << b.cliente
                     << " - " << formatoPesos(b.total) << endl;
            else
                cout << "No hay boletas para anular." << endl;
        } else if (op == 8) {
            cout << "Boletas del dia (la mas reciente arriba):" << endl;
            boletas.mostrar();
            cout << "Total vendido: " << formatoPesos(boletas.totalVendido()) << endl;
        } else if (op == 9) {
            // Solo se admite cargar la muestra en una sesion aun sin actividad.
            if (contadorTurno > 0 || numBoleta > 0 || hayCliente || !pedido.estaVacia()) {
                cout << "Los datos de ejemplo solo se cargan al iniciar el dia." << endl;
            } else {
                boletas.push(Boleta{++numBoleta, "Sra. Rosa", 21480});
                boletas.push(Boleta{++numBoleta, "Don Pedro", 15990});
                fila.InsertarQ("Turno " + to_string(++contadorTurno) + ": Marta");
                fila.InsertarQ("Turno " + to_string(++contadorTurno) + ": Luis");
                fila.InsertarQ("Turno " + to_string(++contadorTurno) + ": Camila");
                cout << "Cargados: 3 clientes en la fila y 2 boletas del dia." << endl;
            }
        }
    }
    cout << "Local cerrado. Total vendido: " << formatoPesos(boletas.totalVendido()) << endl;
    return 0;
}