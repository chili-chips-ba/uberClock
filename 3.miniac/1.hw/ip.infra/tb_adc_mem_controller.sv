//==============================================================================
// ADC_MEM_CONTROLLER Test Bench 
//==============================================================================
`timescale 1ns / 1ps

module tb_adc_mem_controller;

    // Definisanje originalnih parametara za TESTBENCH
    localparam CLK_PERIOD      = 10ns;
    
    // Adrese:
    localparam ADDR_BITS    = 13;
    localparam ADDR_START   = 13'h800;
    localparam ADDR_SPAN    = 13'h1000; // 4096 uzoraka
    localparam ADDR_END     = ADDR_START + ADDR_SPAN - 1; // 0x17FF

    // Signali prema DUT (Device Under Test)
    logic        sys_clk;
    logic        sys_rst_n;
    logic [31:0] adc_sample_in;
    logic        csr_start_i;

    // Signali iz DUT-a
    logic        csr_done_o;
    logic        adc_we_o;
    logic [31:0] adc_data_o;
    logic [12:0] adc_addr_o;

    // INSTANCIJACIJA KONTROLERA (DUT)
    adc_mem_controller DUT (
        .sys_clk        (sys_clk),
        .sys_rst_n      (sys_rst_n),
        .adc_sample_in  (adc_sample_in),
        .csr_start_i    (csr_start_i),
        .csr_done_o     (csr_done_o),
        .adc_we_o       (adc_we_o),
        .adc_data_o     (adc_data_o),
        .adc_addr_o     (adc_addr_o)
    );

    // Generisanje Sata (Clock Generation)
    initial begin
        sys_clk = 0;
        forever #(CLK_PERIOD / 2) sys_clk = ~sys_clk;
    end

    // Monitoring ključnih signala (za detaljniji VCD log)
    initial begin
        $dumpfile("adc_controller.vcd");
        $dumpvars(0, tb_adc_mem_controller);
    end

    // --- Glavna Test Sekvenca ---
    initial begin
        $display("-------------------------------------------------------");
        $display("POCETAK TESTBENCHA za ADC_MEM_CONTROLLER (PUNA VELICINA)");
        $display("TEST BAFER VELICINE: %0d uzoraka (0x%h do 0x%h)",
                 ADDR_SPAN, ADDR_START, ADDR_END);
        $display("-------------------------------------------------------");

        // 1. Inicijalizacija i Reset
        sys_rst_n     = 1'b0;
        csr_start_i   = 1'b0;
        adc_sample_in = 32'hFEED_FACE; 

        repeat (5) @(posedge sys_clk); // Drzanje reseta
        sys_rst_n     = 1'b1;
        $display("@%0t: Reset zavrsen. Adresa bi trebala biti 0x%h.", $time, ADDR_START);

        // 2. PRVA AKVIZICIJA: Punjenje bafera (4096 uzoraka)
        @(posedge sys_clk);
        csr_start_i   = 1'b1; // START puls
        adc_sample_in = 32'hA000_0000;
        $display("@%0t: START puls aktiviran (Akvizicija 1).", $time);

        @(posedge sys_clk);
        csr_start_i   = 1'b0; // Resetovanje START pulsa
        adc_sample_in = 32'hA000_0001; // Prvi uzorak

        // Simulacija punjenja bafera: Pustamo simulaciju da tece 4096 ciklusa
        // (plus dva ciklusa vise za provjeru DONE i povratka adrese)
        
        $display("@%0t: Pocinje prikupljanje 4096 uzoraka. Cekamo do kraja (0x%h)...", $time, ADDR_END);
        
        // Brzo preskakanje do kraja bafera (4096 ciklusa)
        for (int i = 0; i < ADDR_SPAN; i++) begin
            @(posedge sys_clk);
            adc_sample_in = adc_sample_in + 32'h1;
        end

        // Ciklus kada se pise zadnji uzorak (0x17FF)
        $display("@%0t: Pisanje na zadnju adresu (0x%h). Ocekujemo DONE u sledecem ciklusu.", $time, adc_addr_o);

        // Sljedeci ciklus (nakon pisanja na 0x17FF): DONE treba da se postavi, WE da se iskljuci.
        @(posedge sys_clk); 
        
        $display("--- VERIFIKACIJA ---");
        
        if (csr_done_o == 1'b1)
            $display("@%0t: **TEST 1 USPIO:** csr_done_o je postavljen na 1.", $time);
        else
            $error("@%0t: **TEST 1 GRESKA:** csr_done_o nije postavljen.", $time);
            
        if (adc_we_o == 1'b0)
            $display("@%0t: adc_we_o je ispravno zaustavljen.", $time);
        else 
            $error("@%0t: GRESKA: adc_we_o bi trebao biti 0.", $time);

        if (adc_addr_o == ADDR_START)
            $display("@%0t: Adresa se vratila na pocetak (0x%h). Ring Buffer logika OK.", $time, ADDR_START);
        else 
            $error("@%0t: GRESKA: Adresa bi trebala biti 0x%h.", $time, ADDR_START);

        // 3. DRUGA AKVIZICIJA: Provjera resetovanja
        @(posedge sys_clk);
        $display("-------------------------------------------------------");
        $display("@%0t: Pokretanje nove akvizicije (Akvizicija 2) za provjeru reseta.", $time);

        csr_start_i   = 1'b1; // START puls
        adc_sample_in = 32'hB000_0000; // Novi uzorak

        @(posedge sys_clk);
        if (csr_done_o == 1'b0)
            $display("@%0t: csr_done_o je ispravno resetovan START pulsom.", $time);
        else 
            $error("@%0t: GRESKA: csr_done_o nije resetovan.", $time);
            
        csr_start_i   = 1'b0; 

        // Pustanje simulacije da traje jos 5 ciklusa nakon pokretanja
        repeat (5) @(posedge sys_clk);

        $display("-------------------------------------------------------");
        $display("@%0t: Kraj simulacije.", $time);
        $finish;
    end

endmodule
