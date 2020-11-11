`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: KTH Royah Institute of Technology
// Engineer: Seyed Samie Mostafavi
// 
// Create Date: 05/07/2020 03:42:31 PM
// Design Name: 
// Module Name: pressure_logic
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module pressure_logic(
    input [63:0] COUNTER_TS,
    input ENABLE,
    input VALID,
    input [63:0] FR_COUNTER,
    input CLK,
    output EN_OUT,
    output PR_OUT
    );
    
    // 0. At first, enable is 1 (default). valid is 0. en_out is 1. pr_out is 1
    // 1. The driver de-asserts enable.                en_out is 0, pr_out is 1 
    // 2. Then the driver writes fr_counter.           en_out is 0, pr_out is 1
    // 3. Driver asserts valid.                        en_out is 1, pr_out is (FR_COUNTER >= COUNTER_TS). (first 0, then 1)
    // 4. Driver asserts enable, de-assetts valid.     en_out is 1. pr_out is 1
    
    assign EN_OUT = ENABLE | VALID;
    assign PR_OUT = (VALID)?( FR_COUNTER <= COUNTER_TS ):(1);
    
endmodule
