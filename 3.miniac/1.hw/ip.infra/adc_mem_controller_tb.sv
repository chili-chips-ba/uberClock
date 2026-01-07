//==========================================================================
// Testbench za adc_mem_controller
// Fokus: Prikaz toka State Machine (FSM) i signala za RAM upis.
//==========================================================================
`timescale 1ns / 1ps

module adc_mem_controller_tb;

    // Parametri Testbencha
    localparam CLK_PERIOD = 15.385; // 65 MHz
    localparam NUM_SAMPLES = 4096; // 4K uzoraka
    localparam ADDR_START = 13'h400; 

    // Signali za UUT (Unit Under Test)
    logic         sys_clk;
    logic         sys_rst_n;
    logic [31:0]  adc_sample_in;
    logic         csr_start_i;
    wire          csr_done_o;
    wire          adc_we_o;
    wire [31:0]   adc_data_o;
    wire [12:0]   adc_addr_o;
    
    // Brojac za simulaciju
    integer       sample_count;
    
    // Instanciranje Modula pod Testom (UUT)
    adc_mem_controller uut (
        .sys_clk       (sys_clk),
        .sys_rst_n     (sys_rst_n),
        .adc_sample_in (adc_sample_in),
        .csr_start_i   (csr_start_i),
        .csr_done_o    (csr_done_o),
        .adc_we_o      (adc_we_o),
        .adc_data_o    (adc_data_o),
        .adc_addr_o    (adc_addr_o)
    );

    // Generisanje Sata
    always begin
        sys_clk = 1'b0;
        #(CLK_PERIOD / 2) sys_clk = 1'b1;
        #(CLK_PERIOD / 2);
    end

    // --- Kontinuirano pracenje kljucnih signala ---
    // Prikazujemo FSM stanje DIREKTNO preko uut.acq_state_r (hijerarhijski pristup)
    
    initial begin
        // %s format za enumeraciju automatski ispisuje simbolicko ime stanja (IDLE, RUNNING, DONE)
        $monitor("[%0t] FSM Stanje: %s | WE=%b | DONE=%b | ADDR=0x%h | DATA_OUT=0x%h | IN=0x%h", 
            $time, 
            uut.acq_state_r, // NOVO: Direktno pracenje interne varijable stanja
            adc_we_o, csr_done_o, adc_addr_o, adc_data_o, adc_sample_in);
    end

    // --- Inicijalizacija i Test Sekvenca ---
    initial begin
        $display("-------------------------------------------------------");
        $display("Pocetak simulacije ADC Memory Controller-a");
        $display("-------------------------------------------------------");
        
        $dumpfile("adc_mem_controller.vcd");
        $dumpvars(0, adc_mem_controller_tb);
        
        // Inicijalne vrednosti
        sys_rst_n     = 1'b0; // Reset aktivan (Low)
        csr_start_i   = 1'b0;
        adc_sample_in = 32'hFEED_FEED; 
        sample_count  = 0;
        
        // 1. Reset
        @(posedge sys_clk);
        @(posedge sys_clk);
        sys_rst_n = 1'b1; // Reset neaktivan
        $display("[%0t] Izlazak iz reset stanja. Ocekivano: IDLE", $time);
        
        // 2. Start Akvizicije (Puls)
        @(posedge sys_clk);
        csr_start_i = 1'b1; // Start puls
        $display("[%0t] START puls: csr_start_i = 1", $time);

        @(posedge sys_clk);
        csr_start_i = 1'b0; // Spustanje pulsa
        $display("[%0t] START puls: csr_start_i = 0. Ocekivano: RUNNING", $time);

        // 3. Akvizicija - 4096 Uzoraka
        
        // Akvizicija je aktivna (RUNNING)
        while (sample_count < NUM_SAMPLES) begin
            
            // Postavljamo ulazni podatak za sljedeci ciklus
            adc_sample_in = sample_count; 
            
            @(posedge sys_clk); // Sacekaj clock edge (Upis se dogadja ovdje)
            
            sample_count++;
        end

        // 4. Provera DONE stanja (posljednji uzorak je pisan)
        
        // Postavi posljednju vrednost (nece biti upisana u RAM jer je brojac pun)
        adc_sample_in = NUM_SAMPLES; 

        @(posedge sys_clk); // Ciklus nakon poslednjeg upisa
        
        $display("[%0t] Akvizicija Zavrsena. Upisano %0d uzoraka. Ocekivano: DONE", $time, NUM_SAMPLES);
        
        // Sacekaj par ciklusa u DONE stanju
        repeat (3) @(posedge sys_clk);
        
        // 5. Test Ponovnog Pokretanja
        
        // Novi Start puls
        @(posedge sys_clk);
        csr_start_i = 1'b1;
        
        @(posedge sys_clk);
        csr_start_i = 1'b0; 
        
        $display("[%0t] Druga START akvizicija. Ocekivano: RUNNING", $time);
        
        // Pusti da se ponovno pokretanje izvrši
        repeat (5) @(posedge sys_clk);


        // 6. Zavrsetak Simulacije
        repeat (10) @(posedge sys_clk);
        
        $display("-------------------------------------------------------");
        $display("SIMULACIJA ZAVRSENA: Log toka FSM-a je prikazan iznad.");
        $display("-------------------------------------------------------");
        
        $stop;
    end

endmodule
