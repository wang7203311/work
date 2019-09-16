module explored_control(
input logic Clk,launch,Reset,
output logic explored,
input logic[9:0] missle_Y_Pos_in
);
//logic[5:0] state_count;
//logic[5:0] next_state_count;
//logic in_p;
enum logic[4:0]{Wait,Done,in_process} state, next_state;


always_ff @ (posedge Clk)
	begin
		if (Reset)
			begin
			state <= Wait;
//			state_count <= 6'b0;
			end
		else 
			begin
			state <= next_state;
//			state_reg <= next_state_reg;
//			state_count <= next_state_count;
			end
	end

always_comb
begin
//next_state_count = state_count;
explored = 1'b1;
unique case(state)
Wait:
	if(launch == 1'b1)
	next_state = in_process;
	else
	next_state = Wait;
Done:
	next_state = Wait;
in_process:
	if(missle_Y_Pos_in < 10'd390)
	next_state = in_process;
	else
	next_state = Done;
endcase
//
case(state)
Wait:begin
		explored = 1'b1;
//		next_state_count = 6'd0;
		end
Done:
	begin
	explored = 1'b1;
//	next_state_count = 6'd0;
	end
in_process:
	begin
	explored = 1'b0;
//	next_state_count = state_count + 6'b0001;
	end
default:;
endcase
end
endmodule