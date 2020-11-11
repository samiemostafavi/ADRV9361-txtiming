
`timescale 1 ns / 1 ps

	module tx_timing_v1_0 #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line


		// Parameters of Axi Slave Bus Interface S00_AXI
		parameter integer C_S00_AXI_DATA_WIDTH	= 32,
		parameter integer C_S00_AXI_ADDR_WIDTH	= 6
	)
	(
		// Users to add ports here
        input wire [63:0] counter_ts,
        input wire [1:0] active_id,
        input wire fifo_en_in,
        input wire [63:0] dout_in,
        input wire clk_rf,
        output wire fifo_en_out,
        output wire [63:0] dout_out,
        output wire [63:0] load_ts,
        output wire load_en,
		// User ports ends
		// Do not modify the ports beyond this line


		// Ports of Axi Slave Bus Interface S00_AXI
		input wire  s00_axi_aclk,
		input wire  s00_axi_aresetn,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
		input wire [2 : 0] s00_axi_awprot,
		input wire  s00_axi_awvalid,
		output wire  s00_axi_awready,
		input wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
		input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb,
		input wire  s00_axi_wvalid,
		output wire  s00_axi_wready,
		output wire [1 : 0] s00_axi_bresp,
		output wire  s00_axi_bvalid,
		input wire  s00_axi_bready,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
		input wire [2 : 0] s00_axi_arprot,
		input wire  s00_axi_arvalid,
		output wire  s00_axi_arready,
		output wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
		output wire [1 : 0] s00_axi_rresp,
		output wire  s00_axi_rvalid,
		input wire  s00_axi_rready
	);
	
	wire [C_S00_AXI_DATA_WIDTH-1:0] gpio0_0;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio0_1;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio1_0;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio1_1;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio2_0;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio2_1;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio3_0;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio3_1;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio4_0;
    wire [C_S00_AXI_DATA_WIDTH-1:0] gpio4_1;
    wire signed [63:0] load_ts_axi;
    wire [31:0] load_en_axi;
    
    wire [3:0] en_out;
    wire [3:0] pr_out;
    
    reg en_sel = 1;
    reg pr_sel = 1;
    
    reg tmp_out0;
    wire out_sig_pre;
    wire out_sig;
    	
// Instantiation of Axi Bus Interface S00_AXI
	tx_timing_v1_0_S00_AXI # ( 
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) tx_timing_v1_0_S00_AXI_inst (
	    .GPIO0_0(gpio0_0),
	    .GPIO0_1(gpio0_1),
	    .GPIO1_0(gpio1_0),
	    .GPIO1_1(gpio1_1),
	    .GPIO2_0(gpio2_0),
        .GPIO2_1(gpio2_1),
        .GPIO3_0(gpio3_0),
        .GPIO3_1(gpio3_1),
        .GPIO4_0(gpio4_0),
        .GPIO4_1(gpio4_1),
        .GPIO5_0(counter_ts[31:0]),
        .GPIO5_1(counter_ts[63:32]),
        .GPIO6_0(load_ts_axi[31:0]),
        .GPIO6_1(load_ts_axi[63:32]),
        .GPIO6_2(load_en_axi),
		.S_AXI_ACLK(s00_axi_aclk),
		.S_AXI_ARESETN(s00_axi_aresetn),
		.S_AXI_AWADDR(s00_axi_awaddr),
		.S_AXI_AWPROT(s00_axi_awprot),
		.S_AXI_AWVALID(s00_axi_awvalid),
		.S_AXI_AWREADY(s00_axi_awready),
		.S_AXI_WDATA(s00_axi_wdata),
		.S_AXI_WSTRB(s00_axi_wstrb),
		.S_AXI_WVALID(s00_axi_wvalid),
		.S_AXI_WREADY(s00_axi_wready),
		.S_AXI_BRESP(s00_axi_bresp),
		.S_AXI_BVALID(s00_axi_bvalid),
		.S_AXI_BREADY(s00_axi_bready),
		.S_AXI_ARADDR(s00_axi_araddr),
		.S_AXI_ARPROT(s00_axi_arprot),
		.S_AXI_ARVALID(s00_axi_arvalid),
		.S_AXI_ARREADY(s00_axi_arready),
		.S_AXI_RDATA(s00_axi_rdata),
		.S_AXI_RRESP(s00_axi_rresp),
		.S_AXI_RVALID(s00_axi_rvalid),
		.S_AXI_RREADY(s00_axi_rready)
	);
	
	// Add user logic
	
    // Load the offset timestamp
    // We are expecting the AXI block to keep load_en_axi asserted only for 1 cycle
    // load_ts_axi is signed (it can be a negative offset)
	assign load_ts = counter_ts + load_ts_axi;
	assign load_en = load_en_axi[0];
    
    // Back pressure logic
	pressure_logic back_pressure_0 (
	    .COUNTER_TS(counter_ts),
	    .ENABLE(gpio0_0[0]),
	    .VALID(gpio0_1[0]),
	    .FR_COUNTER({gpio1_1,gpio1_0}),
        .CLK(s00_axi_aclk),
        .EN_OUT(en_out[0]),
        .PR_OUT(pr_out[0])
	);
	
	pressure_logic back_pressure_1 (
        .COUNTER_TS(counter_ts),
        .ENABLE(gpio0_0[1]),
        .VALID(gpio0_1[1]),
        .FR_COUNTER({gpio2_1,gpio2_0}),
        .CLK(s00_axi_aclk),
        .EN_OUT(en_out[1]),
        .PR_OUT(pr_out[1])
    );
    
	pressure_logic back_pressure_2 (
        .COUNTER_TS(counter_ts),
        .ENABLE(gpio0_0[2]),
        .VALID(gpio0_1[2]),
        .FR_COUNTER({gpio3_1,gpio3_0}),
        .CLK(s00_axi_aclk),
        .EN_OUT(en_out[2]),
        .PR_OUT(pr_out[2])
    );
    
    pressure_logic back_pressure_3 (
        .COUNTER_TS(counter_ts),
        .ENABLE(gpio0_0[3]),
        .VALID(gpio0_1[3]),
        .FR_COUNTER({gpio4_1,gpio4_0}),
        .CLK(s00_axi_aclk),
        .EN_OUT(en_out[3]),
        .PR_OUT(pr_out[3])
    );
						
    
	// User logic ends
	
	always @(*)
    case (active_id)
       2'b00: en_sel = en_out[0];
       2'b01: en_sel = en_out[1];
       2'b10: en_sel = en_out[2];
       2'b11: en_sel = en_out[3];
       default: en_sel = 0;
    endcase
    
    always @(*)
    case (active_id)
       2'b00: pr_sel = pr_out[0];
       2'b01: pr_sel = pr_out[1];
       2'b10: pr_sel = pr_out[2];
       2'b11: pr_sel = pr_out[3];
       default: pr_sel = 0;
    endcase
    
    assign out_sig_pre = en_sel & pr_sel;
    
    // In order to resolve 1 clock cycle early 1 -> 0
    always @(posedge clk_rf) 
    begin
       if ( s00_axi_aresetn == 1'b0 )
          begin
             tmp_out0 <= 1'b0;
          end
       else
          begin
             tmp_out0 <= out_sig_pre;
          end
    end

    assign out_sig = out_sig_pre | tmp_out0;
    assign fifo_en_out = fifo_en_in & out_sig;
    assign dout_out = (out_sig)?(dout_in):(64'd0);

	endmodule
