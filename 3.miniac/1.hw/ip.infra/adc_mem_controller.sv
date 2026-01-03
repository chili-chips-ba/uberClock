//==========================================================================
// Copyright (C) 2023 Chili.CHIPS*ba
//--------------------------------------------------------------------------
//                      PROPRIETARY INFORMATION
//
// The information contained in this file is the property of CHILI CHIPS LLC.
// Except as specifically authorized in writing by CHILI CHIPS LLC, the holder
// of this file: (1) shall keep all information contained herein confidential;
// and (2) shall protect the same in whole or in part from disclosure and
// dissemination to all third parties; and (3) shall use the same for operation
// and maintenance purposes only.
//--------------------------------------------------------------------------
// Description:
// DPRAM Write Controller for ADC Data.
// Implements a 4096-word (4K) acquisition sequence, writing 32-bit ADC samples
// to the assigned memory space (Port 2). Initiated and reset via CPU CSR.
//==========================================================================

module adc_mem_controller (
    // Clock & Reset
    input  logic         sys_clk,
    input  logic         sys_rst_n,

    // ADC Data Input
    input  logic [31:0]  adc_sample_in,

    // CPU Control Register (CSR)
    input  logic         csr_start_i, // Puls za pokretanje
    output logic         csr_done_o,  // Signalizira da je bafer pun

    // DPRAM Interface (Write only)
    output logic         adc_we_o,    
    output logic [31:0]  adc_data_o,
    output logic [12:0]  adc_addr_o
);

    // Parametri Adresa
    localparam ADDR_BITS     = 13; // 8192 words (od 0 do 8191)
    localparam ADDR_START    = 13'h400; // 2048 (Word address)
    localparam ADDR_SPAN     = 13'h1000; // 4096 words
    localparam ADDR_STOP_AT  = ADDR_START + ADDR_SPAN - 1; // 2048 + 4096 - 1 = 6143 (13'h17FF)

    // State Machine
    typedef enum logic [1:0] {
        IDLE,       // Ceka na start signal (default)
        RUNNING,    // Akvizicija u toku
        DONE        // Akvizicija zavrsena, ceka reset
    } acq_state_t;

    acq_state_t acq_state_r;

    // Unutrasnji Registri
    logic [ADDR_BITS-1:0] write_addr_r; // Adresa za pisanje
    logic                 adc_we_r;     // Generiše WE signal
    logic                 csr_done_r;   // Status: 1 kada je bafer pun

    // Pomoćni signal koji detektuje pisanje na zadnju adresu (puls)
    logic end_of_buffer_write;
    assign end_of_buffer_write = (write_addr_r == ADDR_STOP_AT) && acq_state_r == RUNNING;
    
    // ====================================================================
    // State Machine Logika
    // ====================================================================
    always @(posedge sys_clk) begin
        if (!sys_rst_n) begin
            acq_state_r <= IDLE;
        end else begin
            case (acq_state_r)
                IDLE: begin
                    // Ako CPU posalje START, prelazi u RUNNING
                    if (csr_start_i) begin
                        acq_state_r <= RUNNING;
                    end
                end
                
                RUNNING: begin
                    // Ako smo stigli do zadnje adrese, prelazimo u DONE
                    if (end_of_buffer_write) begin
                        acq_state_r <= DONE;
                    end
                end
                
                DONE: begin
                    // Cekamo da CPU resetuje START (tj. da ga ponovo postavi na 0)
                    // Ako CPU resetuje START, prelazimo u IDLE za novu akviziciju
                    if (csr_start_i == 0) begin
                        acq_state_r <= IDLE;
                    end
                end
            endcase
        end
    end


    // ====================================================================
    // Logika za Adresu (write_addr_r) i WE signal (adc_we_r)
    // ====================================================================
    always @(posedge sys_clk) begin
        if (!sys_rst_n) begin
            write_addr_r <= ADDR_START;
            adc_we_r     <= 1'b0;
            csr_done_r   <= 1'b0;
        end else begin
            
            // Default vrijednosti 
            adc_we_r <= 1'b0;
            
            // Pokretanje/Reset (prema State Machine)
            if (acq_state_r == IDLE && csr_start_i) begin
                write_addr_r <= ADDR_START; // Reset adrese pri STARTU
                csr_done_r   <= 1'b0;       // Reset DONE 
            end
            
            // Pisanje u RAM (samo u RUNNING stanju)
            if (acq_state_r == RUNNING) begin
                
                // 1. Inkrementiraj adresu
                write_addr_r <= write_addr_r + 1'b1;
                
                // 2. Aktiviraj Write Enable
                adc_we_r     <= 1'b1;
                
                // 3. Detekcija kraja akvizicije
                if (end_of_buffer_write) begin
                    // Ovo je posljednji upis.
                    adc_we_r   <= 1'b1; // Zadnji upis je i dalje WE=1
                end
                
            end else if (acq_state_r == DONE) begin
                // Postavi DONE
                csr_done_r <= 1'b1;
                // Postavi adresu na START, ali bez pisanja
                write_addr_r <= ADDR_START;
            end
        end
    end

    // Output Logika 
    assign adc_we_o   = adc_we_r;
    assign adc_data_o = adc_sample_in;
    assign adc_addr_o = write_addr_r;
    assign csr_done_o = csr_done_r;

endmodule
