// EPOS Application Initializer

#include <architecture.h>
#include <utility/heap.h>
#include <machine.h>
#include <system.h>

// compilador coloca todas as variaveis globais e estaticas no seg de dados
// .bss é a parte inferior do segmento de dados
// e no final do segmento da dados é adicionado o _end
extern "C" char _end; // parte do linker script da GNU, pra todas archs

__BEGIN_SYS

class Init_Application
{
private:
    // undeclered heap and stack?
    static const unsigned int HEAP_SIZE = Traits<Application>::HEAP_SIZE;
    static const unsigned int STACK_SIZE = Traits<Application>::STACK_SIZE;

public:
    Init_Application() {
        db<Init>(TRC) << "Init_Application()" << endl;

        // Initialize Application's heap
        db<Init>(INF) << "Initializing application's heap: " << endl;
        if(Traits<System>::multiheap) { // heap in data segment arranged by SETUP
            // ld is eliminating the data segment in some compilations, particularly for RISC-V, and placing _end in the code segment
            // heap é um pointeiro de char (1 byte)
            char *heap;
            if (MMU::align_page(&_end) >= CPU::Log_Addr(Memory_Map::APP_DATA)) {
                // alinhando a heap ao _end
                heap = MMU::align_page(&_end);
            } else {
                heap = CPU::Log_Addr(Memory_Map::APP_DATA);
            }

            // placement new, using the _preheap
            // quem fez o alocamento, foi o setup
            Application::_heap = new (&Application::_preheap[0]) Heap(heap, HEAP_SIZE);
        } else
            for(unsigned int frames = MMU::allocable(); frames; frames = MMU::allocable())
                System::_heap->free(MMU::alloc(frames), frames * sizeof(MMU::Page));
    }
};

// Global object "init_application"  must be linked to the application (not to the system) and there constructed at first.
Init_Application init_application;

__END_SYS
