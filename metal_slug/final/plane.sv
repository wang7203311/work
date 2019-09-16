module plane(
					input         Clk,                // 50 MHz clock
                             Reset,              // Active-high reset signal
                             frame_clk,          // The clock indicating a new frame (~60Hz)
               input [9:0]   DrawX, DrawY,       // Current pixel coordinates
               output logic  is_plane,            // Whether current pixel belongs to plane or background
					input logic[9:0] char_pos,
					output logic[15:0]addr,
					output logic[9:0] start_x,
					output logic[9:0] start_y,
					output logic launch,
					input logic explored
);
	 parameter [9:0] plane_X_Center = 10'd580;  // Center position on the X axis
    parameter [9:0] plane_Y_Center = 10'd20;  // Center position on the Y axis
    parameter [9:0] plane_X_Min = 10'd0;       // Leftmost point on the X axis
    parameter [9:0] plane_X_Max = 10'd639;     // Rightmost point on the X axis
    parameter [9:0] plane_Y_Min = 10'd0;       // Topmost point on the Y axis
    parameter [9:0] plane_Y_Max = 10'd469;     // Bottommost point on the Y axis
    parameter [9:0] plane_X_Step = 10'd1;      // Step size on the X axis
    parameter [9:0] plane_Y_Step = 10'd1;      // Step size on the Y axis
    parameter [9:0] plane_Size_X = 10'd10;        // plane size
	 parameter [9:0] plane_Size_Y = 10'd10;
	
	 logic [9:0] plane_X_Pos, plane_X_Motion, plane_Y_Pos, plane_Y_Motion;
    logic [9:0] plane_X_Pos_in, plane_X_Motion_in, plane_Y_Pos_in, plane_Y_Motion_in;
	 
	 logic frame_clk_delayed, frame_clk_rising_edge;
    always_ff @ (posedge Clk) begin
        frame_clk_delayed <= frame_clk;
        frame_clk_rising_edge <= (frame_clk == 1'b1) && (frame_clk_delayed == 1'b0);
    end
	 always_ff @ (posedge Clk)
    begin
        if (Reset)
        begin
            plane_X_Pos <= plane_X_Center;
            plane_Y_Pos <= plane_Y_Center;
            plane_X_Motion <= 10'd1;
            plane_Y_Motion <= 10'd0;
        end
        else
        begin
            plane_X_Pos <= plane_X_Pos_in;
            plane_Y_Pos <= plane_Y_Pos_in;
            plane_X_Motion <= plane_X_Motion_in;
            plane_Y_Motion <= plane_Y_Motion_in;
        end
    end
	 logic[1:0] compare_bit;
	 always_comb
	 begin
//	 launch = 1'b0;
	 start_x = plane_X_Pos_in;
	 start_y = plane_Y_Pos_in;
	 if(plane_X_Pos_in < (char_pos))
	 compare_bit = 2'b00;
	 else if(plane_X_Pos_in > (char_pos))
	 compare_bit = 2'b01;
	 else
	 begin
	 compare_bit = 2'b11;
//	 launch = 1'b1;
	 end
	 end
	 
	 always_comb
	 begin
	 if(plane_X_Pos_in > char_pos && plane_X_Pos_in < char_pos + 10'd3 && explored == 1'b1)
	 launch = 1'b1;
	 else if(plane_X_Pos_in == char_pos && explored == 1'b1)
	 launch = 1'b1;
	 else if(plane_X_Pos_in < char_pos && plane_X_Pos_in > char_pos - 10'd2 && explored == 1'b1)
	 launch = 1'b1;
	 else
	 launch = 0;
	 end
	 
	 always_comb
    begin
        // By default, keep motion and position unchanged
        plane_X_Pos_in = plane_X_Pos;
        plane_Y_Pos_in = plane_Y_Pos;
		  plane_X_Motion_in = plane_X_Motion;
		  plane_Y_Motion_in = plane_Y_Motion;
        
        // Update position and motion only at rising edge of frame clock
        if (frame_clk_rising_edge)
        begin
            // Be careful when using comparators with "logic" datatype because compiler treats 
            //   both sides of the operator as UNSIGNED numbers.
            // e.g. plane_Y_Pos - plane_Size <= plane_Y_Min 
            // If plane_Y_Pos is 0, then plane_Y_Pos - plane_Size will not be -4, but rather a large positive number.
            if( plane_Y_Pos + plane_Size_Y >= plane_Y_Max )  // plane is at the bottom edge, BOUNCE!
                plane_Y_Motion_in = (~(plane_Y_Step) + 1'b1);  // 2's complement.
//					 plane_X_Motion_in = 10'd0;
            else if ( plane_Y_Pos <= plane_Y_Min + plane_Size_Y )  // plane is at the top edge, BOUNCE!
                plane_Y_Motion_in = plane_Y_Step;
//					 plane_X_Motion_in = 10'd0;
            // TODO: Add other boundary detections and handle keypress here.
				else if(plane_X_Pos + plane_Size_X >= plane_X_Max) //right edge
					  plane_X_Motion_in = ((~(plane_X_Step) + 1'b1));
//					  plane_Y_Motion_in = 10'd0;
				else if(plane_X_Pos <= plane_Y_Min + plane_Size_X)
						plane_X_Motion_in = plane_X_Step;
//						plane_Y_Motion_in = 10'd0;
		  
				case(compare_bit)
				2'b00://plane is on left side
				plane_X_Motion_in = plane_X_Step;
				2'b01://plane is on right side
				plane_X_Motion_in = ((~(plane_X_Step) + 1'b1));
				default:
						begin
						plane_X_Motion_in = 10'd0;
						plane_Y_Motion_in = 10'd0;
						end
				endcase
			   plane_X_Pos_in = plane_X_Pos + plane_X_Motion;
            plane_Y_Pos_in = plane_Y_Pos + plane_Y_Motion;
        end
		end

		
		int DistX, DistY, Size;
    assign DistX = DrawX - plane_X_Pos + 10'd10;
    assign DistY = DrawY - plane_Y_Pos + 10'd10;
//    assign Size = plane_Size;
    always_comb begin
//        if ( ( DistX*DistX + DistY*DistY) <= (Size*Size) ) 
			if(DistX < 10'd20 && DistX > 10'd0 && DistY < 10'd20 && DistY > 10'd0)
            is_plane = 1'b1;
        else
            is_plane = 1'b0;
        /* The plane's (pixelated) circle is generated using the standard circle formula.  Note that while 
           the single line is quite powerful descriptively, it causes the synthesis tool to use up three
           of the 12 available multipliers on the chip! */
    end
		
	assign addr = DistX + DistY*16'd20;
endmodule
