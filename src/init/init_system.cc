// EPOS System Initializer

#include <utility/random.h>
#include <machine.h>
#include <memory.h>
#include <system.h>
#include <process.h>

__BEGIN_SYS

class Init_System
{
private:
    static const unsigned int HEAP_SIZE = Traits<System>::HEAP_SIZE;

public:
    Init_System() {
        db<Init>(TRC) << "Init_System()" << endl;

        db<Init>(INF) << "Init:si=" << *System::info() << endl;

        db<Init>(INF) << "Initializing the architecture: " << endl;
        CPU::init();

        db<Init>(INF) << "Initializing system's heap: " << endl;
        if(Traits<System>::multiheap) {
            // Segment retona um pedaço físico de memoria, que foi alocado pela MMU (é alocado dentro da classe Segment)
            System::_heap_segment = new (&System::_preheap[0]) Segment(HEAP_SIZE, Segment::Flags::SYS);
            char * heap;

            // Acessa os métodos do address_space(não cria um novo) a partir da tabela de paginas primaria MMU::current. 
            // O setup que pega o registrador que está apontando para a tabela de paginas
            // Atacha o segmento no lugar de _heap_segment
            if(Memory_Map::SYS_HEAP == Traits<Machine>::NOT_USED) {
                heap = Address_Space(MMU::current()).attach(System::_heap_segment);
            }
            else {
                // Aloca um segmento no local do SYS_HEAP (endereço lógico)
                // há a possibilidade de retornar um void pointer,
                // mas isso não acontece porque é a primeira coisa a ser feita
                heap = Address_Space(MMU::current()).attach(System::_heap_segment, Memory_Map::SYS_HEAP);
            }
            if(!heap) {
                db<Init>(ERR) << "Failed to initialize the system's heap!" << endl;
            }
            // aloca a _heap no offset de um Segment
            System::_heap = new (&System::_preheap[sizeof(Segment)]) Heap(heap, System::_heap_segment->size());
        } else
            // Caso só tenha uma heap de sistema
            // utiliza o espaço da preheap para criar uma heap
            // MMU::pages recebe o tamanho e retorna a quantidade de bytes
            // para NO_MMU, o tamanho da página é de 1 byte
            // MMU::aloc retorna um ponteiro físico, só funciona porque está usando o NO_MMU
            System::_heap = new (&System::_preheap[0]) Heap(MMU::alloc(MMU::pages(HEAP_SIZE)), HEAP_SIZE);

        db<Init>(INF) << "Initializing the machine: " << endl;
        Machine::init();

        db<Init>(INF) << "Initializing system abstractions: " << endl;
        System::init();

        // Randomize the Random Numbers Generator's seed
        if(Traits<Random>::enabled) {
            db<Init>(INF) << "Randomizing the Random Numbers Generator's seed." << endl;
            if(Traits<TSC>::enabled)
                Random::seed(TSC::time_stamp());

            if(!Traits<TSC>::enabled)
                db<Init>(WRN) << "Due to lack of entropy, Random is a pseudo random numbers generator!" << endl;
        }

        // Initialization continues at init_end
    }
};

// Global object "init_system" must be constructed first.
Init_System init_system;

__END_SYS
