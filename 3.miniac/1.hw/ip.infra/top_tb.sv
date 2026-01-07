`timescale 1ns / 1ps

module top_tb;

    // --- Parametri ---
    localparam CLK_PERIOD = 15.385; // 65 MHz period

    // --- Signali za simulaciju ploce ---
    logic clk_p, clk_n;
    logic rst_n;
    logic uart_rx = 1'b1;
    wire  uart_tx;
    logic user_key1 = 1'b1;
    logic user_key2 = 1'b1;
    wire [3:0] led;

    // ADC ulazi
    logic [11:0] ad9238_data_ch0;
    logic [11:0] ad9238_data_ch1;

    // --- Instanciranje TOP modula ---
    top uut (
        .clk_p(clk_p), .clk_n(clk_n), .rst_n(rst_n),
        .uart_rx(uart_rx), .uart_tx(uart_tx),
        .user_key1(user_key1), .user_key2(user_key2),
        .led(led),
        .ad9238_data_ch0(ad9238_data_ch0), .ad9238_data_ch1(ad9238_data_ch1)
        // ... ostali signali (DAC, itd) ostaju isti kao u tvom top.sv
    );

    // --- Generisanje diferencijalnog sata (65 MHz) ---
    // 65 MHz sat: period = 15.385 ns
    always begin
        clk_p = 1'b0;
        clk_n = 1'b1;
        #(15.385 / 2);
        clk_p = 1'b1;
        clk_n = 1'b0;
        #(15.385 / 2);
    end

    // --- MONITOR: Ispisuje sta se desava UNUTAR kontrolera ---
    initial begin
    // 1. Inicijalizacija
    rst_n = 1'b0;           // Krenimo od aktivnog reseta
    ad9238_data_ch0 = 12'hAAA;
    ad9238_data_ch1 = 12'h555;
    
    // 2. Forsiranje unutrašnjeg sata i reseta 
    // (Ovo radimo jer clk_rst_gen modul često "uguši" simulaciju)
    force uut.sys_clk = clk_p; 
    force uut.sys_rst_n = rst_n;

    #200;                   // Drži reset 200ns
    rst_n = 1'b1;           // Pusti reset
    force uut.sys_rst_n = 1'b1;
    $display("[%0t] Reset deaktiviran.", $time);

    #500;                   // Pusti sat da "odradi" malo u IDLE stanju

    // 3. SLANJE START SIGNALA
    // Bitno: Start mora biti sinhronizovan sa ivicom sata!
    @(posedge uut.sys_clk);
    force uut.csr_start_in = 1'b1;
    
    @(posedge uut.sys_clk);
    force uut.csr_start_in = 1'b0; // Spusti start nakon jednog ciklusa
    $display("[%0t] START impuls poslan.", $time);

    // 4. Dinamička promjena ADC podataka dok sistem radi
    // Ovo će ti pokazati da se različite vrijednosti upisuju u RAM
    repeat(100) begin
        @(posedge uut.sys_clk);
        ad9238_data_ch0 <= ad9238_data_ch0 + 1;
        ad9238_data_ch1 <= ad9238_data_ch1 + 1;
    end

    // 5. Provjera da li je adresa krenula
    #1000;
    if (uut.adc_addr > 13'h0400)
        $display("[%0t] USPJEH: Adresa se inkrementira! Trenutna: %h", $time, uut.adc_addr);
    else
        $display("[%0t] GRESKA: Adresa je i dalje 0400. Provjeri FSM uslove.", $time);

    #5000;
    $stop;
end

endmodule