//-------------------------------------------------------------------------
//      lab8.sv                                                          --
//      Christine Chen                                                   --
//      Fall 2014                                                        --
//                                                                       --
//      Modified by Po-Han Huang                                         --
//      10/06/2017                                                       --
//                                                                       --
//      Fall 2017 Distribution                                           --
//                                                                       --
//      For use with ECE 385 Lab 8                                       --
//      UIUC ECE Department                                              --
//-------------------------------------------------------------------------


module lab8( input               CLOCK_50,
             input        [3:0]  KEY,          //bit 0 is set up as Reset
             output logic [6:0]  HEX0, HEX1,
             // VGA Interface 
             output logic [7:0]  VGA_R,        //VGA Red
                                 VGA_G,        //VGA Green
                                 VGA_B,        //VGA Blue
             output logic        VGA_CLK,      //VGA Clock
                                 VGA_SYNC_N,   //VGA Sync signal
                                 VGA_BLANK_N,  //VGA Blank signal
                                 VGA_VS,       //VGA virtical sync signal
                                 VGA_HS,       //VGA horizontal sync signal
             // CY7C67200 Interface
             inout  wire  [15:0] OTG_DATA,     //CY7C67200 Data bus 16 Bits
             output logic [1:0]  OTG_ADDR,     //CY7C67200 Address 2 Bits
             output logic        OTG_CS_N,     //CY7C67200 Chip Select
                                 OTG_RD_N,     //CY7C67200 Write
                                 OTG_WR_N,     //CY7C67200 Read
                                 OTG_RST_N,    //CY7C67200 Reset
             input               OTG_INT,      //CY7C67200 Interrupt
             // SDRAM Interface for Nios II Software
             output logic [12:0] DRAM_ADDR,    //SDRAM Address 13 Bits
             inout  wire  [31:0] DRAM_DQ,      //SDRAM Data 32 Bits
             output logic [1:0]  DRAM_BA,      //SDRAM Bank Address 2 Bits
             output logic [3:0]  DRAM_DQM,     //SDRAM Data Mast 4 Bits
             output logic        DRAM_RAS_N,   //SDRAM Row Address Strobe
                                 DRAM_CAS_N,   //SDRAM Column Address Strobe
                                 DRAM_CKE,     //SDRAM Clock Enable
                                 DRAM_WE_N,    //SDRAM Write Enable
                                 DRAM_CS_N,    //SDRAM Chip Select
                                 DRAM_CLK,     //SDRAM Clock
				 // audio part
				 input  AUD_BCLK,
				 input  AUD_ADCDAT,
				 output AUD_DACDAT,
				 input  AUD_DACLRCK, 
				 input  AUD_ADCLRCK,
				 output I2C_SDAT,
				 output I2C_SCLK,
				 output AUD_XCK
                    );
    
    logic Reset_h, Clk;
    logic [7:0] keycode;
    
    assign Clk = CLOCK_50;
    always_ff @ (posedge Clk) begin
        Reset_h <= ~(KEY[0]);        // The push buttons are active low
    end
    
    logic [1:0] hpi_addr;
    logic [15:0] hpi_data_in, hpi_data_out;
    logic hpi_r, hpi_w, hpi_cs, hpi_reset;
    
    // Interface between NIOS II and EZ-OTG chip
    hpi_io_intf hpi_io_inst(
                            .Clk(Clk),
                            .Reset(Reset_h),
                            // signals connected to NIOS II
                            .from_sw_address(hpi_addr),
                            .from_sw_data_in(hpi_data_in),
                            .from_sw_data_out(hpi_data_out),
                            .from_sw_r(hpi_r),
                            .from_sw_w(hpi_w),
                            .from_sw_cs(hpi_cs),
                            .from_sw_reset(hpi_reset),
                            // signals connected to EZ-OTG chip
                            .OTG_DATA(OTG_DATA),    
                            .OTG_ADDR(OTG_ADDR),    
                            .OTG_RD_N(OTG_RD_N),    
                            .OTG_WR_N(OTG_WR_N),    
                            .OTG_CS_N(OTG_CS_N),
                            .OTG_RST_N(OTG_RST_N)
    );
     
     // You need to make sure that the port names here match the ports in Qsys-generated codes.
     nios_system nios_system(
                             .clk_clk(Clk),         
                             .reset_reset_n(1'b1),    // Never reset NIOS
                             .sdram_wire_addr(DRAM_ADDR), 
                             .sdram_wire_ba(DRAM_BA),   
                             .sdram_wire_cas_n(DRAM_CAS_N),
                             .sdram_wire_cke(DRAM_CKE),  
                             .sdram_wire_cs_n(DRAM_CS_N), 
                             .sdram_wire_dq(DRAM_DQ),   
                             .sdram_wire_dqm(DRAM_DQM),  
                             .sdram_wire_ras_n(DRAM_RAS_N),
                             .sdram_wire_we_n(DRAM_WE_N), 
                             .sdram_clk_clk(DRAM_CLK),
                             .keycode_export(keycode),  
                             .otg_hpi_address_export(hpi_addr),
                             .otg_hpi_data_in_port(hpi_data_in),
                             .otg_hpi_data_out_port(hpi_data_out),
                             .otg_hpi_cs_export(hpi_cs),
                             .otg_hpi_r_export(hpi_r),
                             .otg_hpi_w_export(hpi_w),
                             .otg_hpi_reset_export(hpi_reset)
    );
	 
	 
	 // audio part
	 // audio control
	 // logic dig, mark, ON_MINE, CHEAT;
	 logic Init, Init_Finish, data_over;
	 logic [15:0] Ldata, Rdata;
	 
	 audio_controller audio_controller_module( 
														.Clk(CLOCK_50),
														.Reset(Reset_h), 
														.data_over(data_over),
														.LDATA(Ldata), 
														.RDATA(Rdata),
														.Init(Init),
														.Init_Finish(Init_Finish)
														);  
	 audio_interface audio( 
								.LDATA(Ldata), 
								.RDATA(Rdata),          //IN std_logic_vector(15 downto 0); -- parallel external data inputs
								.clk(CLOCK_50), 
								.Reset(Reset_h), 
								.INIT(Init),        //IN std_logic; 
								.INIT_FINISH(Init_Finish),      //OUT std_logic;
								.adc_full(),       //OUT std_logic;
								.data_over(data_over),            //OUT std_logic; -- sample sync pulse
								.AUD_MCLK(AUD_XCK),               //OUT std_logic; -- Codec master clock OUTPUT
								.AUD_BCLK(AUD_BCLK),               //IN std_logic; -- Digital Audio bit clock
								.AUD_ADCDAT(AUD_ADCDAT),      //IN std_logic;
								.AUD_DACDAT(AUD_DACDAT),             //OUT std_logic; -- DAC data line
								.AUD_DACLRCK(AUD_DACLRCK), 
								.AUD_ADCLRCK(AUD_ADCLRCK),           //IN std_logic; -- DAC data left/right select
								.I2C_SDAT(I2C_SDAT),    //OUT std_logic; -- serial interface data line
								.I2C_SCLK(I2C_SCLK),    //OUT std_logic;  -- serial interface clock
								.ADCDATA(   )     //OUT std_logic_vector(31 downto 0)
								);
    
    // Use PLL to generate the 25MHZ VGA_CLK.
    // You will have to generate it on your own in simulation.
    vga_clk vga_clk_instance(.inclk0(Clk), .c0(VGA_CLK));
    
    // TODO: Fill in the connections for the rest of the modules 
	 logic [9:0] DrawX,DrawY;
    VGA_controller vga_controller_instance(.Clk,.Reset(Reset_h),.VGA_HS,.VGA_VS,
	 .VGA_CLK,.VGA_BLANK_N,.VGA_SYNC_N,.DrawX,.DrawY);
    
    // Which signal should be frame_clk?
	 logic is_ball;
    ball ball_instance(.Clk,.Reset(Reset_h),.frame_clk(VGA_VS),.is_ball,.DrawX,.DrawY,.keycode);
    
    color_mapper color_instance(.is_ball,.DrawX,.DrawY,.VGA_R,.VGA_G,.VGA_B);
    
    // Display keycode on hex display
    HexDriver hex_inst_0 (keycode[3:0], HEX0);
    HexDriver hex_inst_1 (keycode[7:4], HEX1);
	 
	 
    
    /**************************************************************************************
        ATTENTION! Please answer the following quesiton in your lab report! Points will be allocated for the answers!
        Hidden Question #1/2:
        What are the advantages and/or disadvantages of using a USB interface over PS/2 interface to
             connect to the keyboard? List any two.  Give an answer in your Post-Lab.
    **************************************************************************************/
endmodule
