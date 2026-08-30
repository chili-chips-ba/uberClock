module adc(
   //sys
   input                            sys_clk,               //system clock negative
   input                            rst_n,                   //reset ,low active

   //ADC
   output                           adc_clk_ch0,          //AD channel 0 sampling clock
   output                           adc_clk_ch1,          //AD channel 1 sampling clock
   input  [11:0]                    adc_data_ch0,         //AD channel 0 data
   input  [11:0]                    adc_data_ch1,         //AD channel 1 data
   output [11:0]             ad_data_ch0,
   output [11:0]                    ad_data_ch1
);


    wire [11:0] adc0_rising,  adc0_falling;
    wire [11:0] adc1_rising,  adc1_falling;

    genvar i;
    generate
      for (i = 0; i < 12; i = i + 1) begin : gen_iddr_ch0
        IDDR #(
          .DDR_CLK_EDGE    ("SAME_EDGE_PIPELINED"), // Q1 and Q2 valid on rising edge
          .INIT_Q1         (1'b0),
          .INIT_Q2         (1'b0)
        ) iddr_ch0_i (
          .Q1  (adc0_rising[i]),       // data from rising edge
          .Q2  (adc0_falling[i]),      // data from falling edge
          .C   (sys_clk),      // ADC clk
          .CE  (1'b1),
          .D   (adc_data_ch0[i]),  // DDR input pin
          .R   (1'b0),
          .S   (1'b0)
        );
      end
      for (i = 0; i < 12; i = i + 1) begin : gen_iddr_ch1
        IDDR #(
          .DDR_CLK_EDGE    ("SAME_EDGE_PIPELINED"),
          .INIT_Q1         (1'b0),
          .INIT_Q2         (1'b0)
        ) iddr_ch1_i (
          .Q1  (adc1_rising[i]),
          .Q2  (adc1_falling[i]),
          .C   (sys_clk),
          .CE  (1'b1),
          .D   (adc_data_ch1[i]),
          .R   (1'b0),
          .S   (1'b0)
        );
      end
    endgenerate

//   assign adc_clk_ch0  = adc_clk;
    ODDR #(
    .DDR_CLK_EDGE           ("SAME_EDGE"           )
    )
    ODDR_clk_ch0
    (
    .Q                      (adc_clk_ch0          ),              // 1-bit DDR output data
    .C                      (sys_clk           ),              // 1-bit clock input
    .CE                     (1'b1                    ),              // 1-bit clock enable input
    .D1                     (1'b1                    ),              // 1-bit data input (associated with C)
    .D2                     (1'b0                    ),              // 1-bit data input (associated with C)
    .R                      (1'b0                    ),              // 1-bit reset input
    .S                      (1'b0                    )               // 1-bit set input
    );

//   assign adc_clk_ch1  = adc_clk;
    ODDR #(
    .DDR_CLK_EDGE           ("SAME_EDGE"           )
    )
    ODDR_clk_ch1
    (
    .Q                      (adc_clk_ch1          ),              // 1-bit DDR output data
    .C                      (sys_clk           ),              // 1-bit clock input
    .CE                     (1'b1                    ),              // 1-bit clock enable input
    .D1                     (1'b1                    ),              // 1-bit data input (associated with C)
    .D2                     (1'b0                    ),              // 1-bit data input (associated with C)
    .R                      (1'b0                    ),              // 1-bit reset input
    .S                      (1'b0                    )               // 1-bit set input
    );

assign ad_data_ch0 = adc0_rising;
assign ad_data_ch1 = adc1_rising;


endmodule
