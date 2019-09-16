module  audio_controller ( output [15:0]  LDATA, RDATA,
									output			Init, 
									input				Init_Finish, Reset, Clk, data_over,
									input 			frame_clk
								  );   
    parameter [9:0] d1 = 8'd44;//8'd65;
	 parameter [9:0] d2 = 8'd50;//8'd73;
	 parameter [9:0] d3_= 8'd52;//8'd78;
	 parameter [9:0] d3 = 8'd55;//8'd82;
	 parameter [9:0] d4 = 8'd59;//8'd87;
	 parameter [9:0] d5 = 8'd66;//8'd98;
	 parameter [9:0] d6 = 8'd73;//8'd110;
	 parameter [9:0] d7 = 8'd83;//8'd123;
	 parameter [9:0] h1 = 8'd88;//8'd131;
	 parameter [9:0] l1 = 8'd33;
	 parameter [9:0] l2 = 8'd101;
	 parameter [9:0] l3 = 8'd28;//8'd37;//
	 parameter [9:0] l4 = 8'd35;//8'd41;//
	 parameter [9:0] l5 = 8'd33;//8'd44;//
	 parameter [9:0] l6 = 8'd36;//8'd49;//
	 parameter [9:0] l7 = 8'd39;//8'd55;//
	 parameter [9:0] l71 = 8'd41;//;
	 parameter [9:0] dx = 8'd00;
	 
int duration, counter;
enum logic [1:0] {Normal} state,next_state;

logic [127:0][7:0] BGM; 
always_comb begin

		BGM[0][7:0  ] = d6;
		BGM[1][7:0  ] = d6;
		BGM[2][7:0  ] = d6;
		BGM[3][7:0  ] = d6;
		BGM[4][7:0  ] = d6;
		BGM[5][7:0  ] = d6;
		
		BGM[6][7:0  ] = d5;
		BGM[7][7:0  ] = d5;
		
		BGM[8][7:0  ] = d3;
		BGM[9][7:0  ] = d3;
		BGM[10][7:0 ] = d3;
		BGM[11][7:0 ] = d3;
		
		BGM[12][7:0 ] = d2;
		BGM[13][7:0 ] = d2;
		BGM[14][7:0 ] = d2;
		BGM[15][7:0 ] = d2;
		
		BGM[16][7:0 ] = d1;
		BGM[17][7:0 ] = d1;
		BGM[18][7:0 ] = d1;
		BGM[19][7:0 ] = d1;
		BGM[20][7:0 ] = d1;
		BGM[21][7:0 ] = d1;
		BGM[22][7:0 ] = d1;
		BGM[23][7:0 ] = d1;
		
		BGM[24][7:0 ] = dx;
		BGM[25][7:0 ] = dx;
		BGM[26][7:0 ] = dx;
		BGM[27][7:0 ] = dx;
		BGM[28][7:0 ] = dx;
		BGM[29][7:0 ] = dx;
		BGM[30][7:0 ] = dx;
		BGM[31][7:0 ] = dx;
		
		BGM[32][7:0 ] = d3;
		BGM[33][7:0 ] = d3;
		BGM[34][7:0 ] = d3;
		BGM[35][7:0 ] = d3;
		BGM[36][7:0 ] = d3;
		BGM[37][7:0 ] = d3;
		
		BGM[38][7:0 ] = d2;
		BGM[39][7:0 ] = d2;
		
		
		BGM[40][7:0 ] = d1;
		BGM[41][7:0 ] = d1;
		BGM[42][7:0 ] = d1;
		BGM[43][7:0 ] = d1;
		
		BGM[44][7:0 ] = l6;
		BGM[45][7:0 ] = l6;
		BGM[46][7:0 ] = l6;
		BGM[47][7:0 ] = l6;
		
		BGM[48][6:0 ] = l5;
		BGM[49][6:0 ] = l5;
		BGM[50][6:0 ] = l5;
		BGM[51][6:0 ] = l5;
		BGM[52][6:0 ] = l5;
		BGM[53][6:0 ] = l5;
		BGM[54][6:0 ] = l5;
		BGM[55][6:0 ] = l5;
		
		BGM[56][6:0 ] = dx;
		BGM[57][6:0 ] = dx;
		BGM[58][6:0 ] = dx;
		BGM[59][6:0 ] = dx;
		BGM[60][6:0 ] = dx;
		BGM[61][6:0 ] = dx;
		BGM[62][6:0 ] = dx;
		BGM[63][6:0 ] = dx;
		
		BGM[64][6:0 ] = l5;
		BGM[65][6:0 ] = l5;
		BGM[66][6:0 ] = l5;
		BGM[67][6:0 ] = l5;
		BGM[68][6:0 ] = l5;
		BGM[69][6:0 ] = l5;
		
		BGM[70][7:0 ] = l6;
		BGM[71][7:0 ] = l6;
		
		BGM[72][6:0 ] = l5;
		BGM[73][6:0 ] = l5;
		BGM[74][6:0 ] = l5;
		BGM[75][6:0 ] = l5;
		
		BGM[76][7:0 ] = l6;
		BGM[77][7:0 ] = l6;
		BGM[78][7:0 ] = l6;
		BGM[79][7:0 ] = l6;
		
		BGM[80][7:0 ] = d1;
		BGM[81][7:0 ] = d1;
		BGM[82][7:0 ] = d1;
		BGM[83][7:0 ] = d1;
		BGM[84][7:0 ] = d1;
		BGM[85][7:0 ] = d1;
		
		BGM[86][7:0 ] = d2;
		BGM[87][7:0 ] = d2;
		
		BGM[88][7:0 ] = d3;
		BGM[89][7:0 ] = d3;
		BGM[90][7:0 ] = d3;
		BGM[91][7:0 ] = d3;
		
		BGM[92][7:0 ] = d5;
		BGM[93][7:0 ] = d5;
		BGM[94][7:0 ] = d5;
		BGM[95][7:0 ] = d5;
		
		BGM[96][7:0 ] = d6;
		BGM[97][7:0 ] = d6;
		BGM[98][7:0 ] = d6;
		BGM[99][7:0 ] = d6;
		BGM[100][7:0 ] = d6;
		BGM[101][7:0 ] = d6;
		
		BGM[102][7:0 ] = d5;
		BGM[103][7:0 ] = d5;
		
		BGM[104][7:0 ] = d3;
		BGM[105][7:0 ] = d3;
		
		BGM[106][7:0 ] = d2;
		BGM[107][7:0 ] = d2;
		
		BGM[108][7:0 ] = d1;
		BGM[109][7:0 ] = d1;
		BGM[110][7:0 ] = d1;
		BGM[111][7:0 ] = d1;
		
		BGM[112][7:0 ] = d2;
		BGM[113][7:0 ] = d2;
		BGM[114][7:0 ] = d2;
		BGM[115][7:0 ] = d2;
		BGM[116][7:0 ] = d2;
		BGM[117][7:0 ] = d2;
		BGM[118][7:0 ] = d2;
		BGM[119][7:0 ] = d2;
		
		BGM[120][6:0 ] = dx;
		BGM[121][6:0 ] = dx;
		BGM[122][6:0 ] = dx;
		BGM[123][6:0 ] = dx;
		BGM[124][6:0 ] = dx;
		BGM[125][6:0 ] = dx;
		BGM[126][6:0 ] = dx;
		BGM[127][6:0 ] = dx;
		 
		
	
end
always_comb
	begin	
	unique case(state)
		Normal:
		begin
		next_state<=Normal;
		end
	endcase
end	
	

always_ff @ (posedge Clk or posedge Reset)
	begin
		if ( Reset ) 
			begin 
				LDATA <= 16'hFFFF;
				RDATA <= 16'hFFFF;
				Init  <= 1'b1;
				counter <= 1;
				duration <= 0;
				state<= Normal;
			end
		else
			begin
	
		state <= next_state;

			if(Init_Finish)
				begin
					duration <= duration + 1;
					if(data_over)
					begin
						RDATA <= RDATA + BGM[counter][6:0];
					end
					else//DO
					begin
						Init <= 1'b0;
						LDATA <= LDATA;
						if(counter < 130)
						begin
							counter <= counter + duration/50000;
							duration <= duration%50000;
						end
						else
						begin
							counter <= 0;
						end
					end
				end
				else//IF
				begin
					Init <= Init;
					LDATA <= LDATA;
					RDATA <= RDATA;
				end
			end
		
	end

endmodule
