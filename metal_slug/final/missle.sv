module missle(
input launch,
output is_missle,
input         Clk,                // 50 MHz clock
                             Reset,              // Active-high reset signal
                             frame_clk,          // The clock indicating a new frame (~60Hz)
               input [9:0]   DrawX, DrawY,       // Current pixel coordinates
output logic[15:0]addr,
input logic[9:0]start_x,
input logic[9:0]start_y,
output logic explored
);
//	 parameter [9:0] missle_X_Center = start_x;  // Center position on the X axis
//    parameter [9:0] missle_Y_Center = start_x;  // Center position on the Y axis
    parameter [9:0] missle_X_Min = 10'd0;       // Leftmost point on the X axis
    parameter [9:0] missle_X_Max = 10'd639;     // Rightmost point on the X axis
    parameter [9:0] missle_Y_Min = 10'd0;       // Topmost point on the Y axis
    parameter [9:0] missle_Y_Max = 10'd469;     // Bottommost point on the Y axis
    parameter [9:0] missle_X_Step = 10'd1;      // Step size on the X axis
    parameter [9:0] missle_Y_Step = 10'd5;      // Step size on the Y axis
    parameter [9:0] missle_Size_X = 10'd4;        // missle size
	 parameter [9:0] missle_Size_Y = 10'd10;
	
	 logic [9:0] missle_X_Pos, missle_X_Motion, missle_Y_Pos, missle_Y_Motion;
    logic [9:0] missle_X_Pos_in, missle_X_Motion_in, missle_Y_Pos_in, missle_Y_Motion_in;
	 
	 logic frame_clk_delayed, frame_clk_rising_edge;
    always_ff @ (posedge Clk) begin
        frame_clk_delayed <= frame_clk;
        frame_clk_rising_edge <= (frame_clk == 1'b1) && (frame_clk_delayed == 1'b0);
    end
	 
	 always_ff @ (posedge Clk)
    begin
        if (launch)
        begin
            missle_X_Pos <= start_x;
            missle_Y_Pos <= start_y;
            missle_X_Motion <= 10'd0;
            missle_Y_Motion <= 10'd1;
//				state_count <= 6'b0;
        end
        else
        begin
            missle_X_Pos <= missle_X_Pos_in;
            missle_Y_Pos <= missle_Y_Pos_in;
            missle_X_Motion <= missle_X_Motion_in;
            missle_Y_Motion <= missle_Y_Motion_in;
//				state_count <= next_state_count;
        end
    end
	 
	 
	 always_comb
    begin
        // By default, keep motion and position unchanged
        missle_X_Pos_in = missle_X_Pos;
        missle_Y_Pos_in = missle_Y_Pos;
		  missle_X_Motion_in = missle_X_Motion;
		  missle_Y_Motion_in = missle_Y_Motion;
        
        // Update position and motion only at rising edge of frame clock
        if (frame_clk_rising_edge)
        begin
            // Be careful when using comparators with "logic" datatype because compiler treats 
            //   both sides of the operator as UNSIGNED numbers.
            // e.g. missle_Y_Pos - missle_Size <= missle_Y_Min 
            // If missle_Y_Pos is 0, then missle_Y_Pos - missle_Size will not be -4, but rather a large positive number.
//            if( missle_Y_Pos + missle_Size_Y >= missle_Y_Max )  // missle is at the bottom edge, BOUNCE!
//                missle_Y_Motion_in = (~(missle_Y_Step) + 1'b1);  // 2's complement.
////					 missle_X_Motion_in = 10'd0;
//            else if ( missle_Y_Pos <= missle_Y_Min + missle_Size_Y )  // missle is at the top edge, BOUNCE!
//                missle_Y_Motion_in = missle_Y_Step;
////					 missle_X_Motion_in = 10'd0;
            // TODO: Add other boundary detections and handle keypress here.
//				if(missle_X_Pos + missle_Size_X >= missle_X_Max) //right edge
//					  missle_X_Motion_in = ((~(missle_X_Step) + 1'b1));
////					  missle_Y_Motion_in = 10'd0;
//				else if(missle_X_Pos <= missle_Y_Min + missle_Size_X)
//						missle_X_Motion_in = missle_X_Step;
////						missle_Y_Motion_in = 10'd0;
		  
			if(missle_Y_Pos < 10'd400)
			 begin
			 missle_X_Motion_in = 10'd0;
			 missle_Y_Motion_in = missle_Y_Step;
			 end
			else
			 begin
		    missle_X_Motion_in = 10'd0;
			 missle_Y_Motion_in =10'd0;
			 end
			 missle_X_Pos_in = missle_X_Pos + missle_X_Motion;
          missle_Y_Pos_in = missle_Y_Pos + missle_Y_Motion;
		end
	end
	
	int DistX, DistY, Size;
   assign DistX = DrawX - missle_X_Pos + 10'd4;
   assign DistY = DrawY - missle_Y_Pos + 10'd10;
//    assign Size = missle_Size;
   always_comb begin
//        if ( ( DistX*DistX + DistY*DistY) <= (Size*Size) ) 
			if(DistX < 10'd8 && DistX > 10'd0 && DistY < 10'd20 && DistY > 10'd0 && missle_Y_Pos < 10'd400)
            is_missle = 1'b1;
        else
            is_missle = 1'b0;
        /* The missle's (pixelated) circle is generated using the standard circle formula.  Note that while 
           the single line is quite powerful descriptively, it causes the synthesis tool to use up three
           of the 12 available multipliers on the chip! */
    end
		
	assign addr = DistX + DistY*16'd8;
//	always_comb
//	begin
//	if(missle_Y_Pos < 10'd400)
//	explored = 1'b0;
//	else
//	explored = 1'b1;
//	end
explored_control control_mis(.Clk,.Reset,.launch,.explored,.missle_Y_Pos_in);


endmodule
