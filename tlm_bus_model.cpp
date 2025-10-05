#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
using namespace sc_core;
using namespace tlm;

// ---------------- CPU (Initiator) ----------------
SC_MODULE(CPU) {
    tlm_utils::simple_initiator_socket<CPU> socket;

    SC_CTOR(CPU) {
        SC_THREAD(run);
    }

    void run() {
        tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;

        unsigned char data[4] = {0};

        trans.set_command(TLM_READ_COMMAND);
        trans.set_address(0x1000);     // Address to read
        trans.set_data_ptr(data);
        trans.set_data_length(4);

        std::cout << "[CPU] Sending READ transaction to Bus at "
                  << sc_time_stamp() << std::endl;

        socket->b_transport(trans, delay);

        std::cout << "[CPU] Transaction completed at "
                  << sc_time_stamp() + delay << std::endl;
    }
};

// ---------------- BUS (Forwarder) ----------------
SC_MODULE(Bus) {
    tlm_utils::simple_target_socket<Bus> in_socket;     // from CPU
    tlm_utils::simple_initiator_socket<Bus> out_socket; // to Memory

    SC_CTOR(Bus) {
        in_socket.register_b_transport(this, &Bus::b_transport);
    }

    void b_transport(tlm_generic_payload &trans, sc_time &delay) {
        std::cout << "[Bus] Received transaction. Decoding address 0x"
                  << std::hex << trans.get_address() << " at "
                  << sc_time_stamp() << std::endl;

        // (Optional) Address decoding logic here
        // In real SoC: decide which target to forward to based on address range
        out_socket->b_transport(trans, delay);  // Forward to memory

        std::cout << "[Bus] Forwarded transaction to Memory at "
                  << sc_time_stamp() + delay << std::endl;
    }
};

// ---------------- MEMORY (Target) ----------------
SC_MODULE(Memory) {
    tlm_utils::simple_target_socket<Memory> socket;
    unsigned char mem[256]; // Small memory array

    SC_CTOR(Memory) {
        socket.register_b_transport(this, &Memory::b_transport);
        for (int i = 0; i < 256; i++) mem[i] = i; // preload data
    }

    void b_transport(tlm_generic_payload &trans, sc_time &delay) {
        sc_dt::uint64 addr = trans.get_address();
        unsigned char *ptr = trans.get_data_ptr();

        if (addr >= 256) {
            std::cerr << "[Memory] ERROR: Invalid address " << addr << std::endl;
            return;
        }

        if (trans.get_command() == TLM_READ_COMMAND) {
            *ptr = mem[addr];
            std::cout << "[Memory] READ request at address 0x" 
                      << std::hex << addr << " -> data=" << std::dec 
                      << (int)*ptr << " at " << sc_time_stamp() << std::endl;
        }

        delay += sc_time(10, SC_NS); // Simulate 10ns delay
    }
};

// ---------------- MAIN ----------------
int sc_main(int, char*[]) {
    CPU cpu("cpu");
    Bus bus("bus");
    Memory mem("mem");

    // Connections
    cpu.socket.bind(bus.in_socket);
    bus.out_socket.bind(mem.socket);

    sc_start();  // Start simulation
    return 0;
}

