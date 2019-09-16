module frameROM(
	input logic[15:0] addr,
	output logic[23:0] data_out,
	input in_range,
	input is_plane,
	input is_missle,
	input logic[9:0]DrawX,DrawY,
	input logic[15:0] plane_addr,
	input logic[15:0] missle_addr
);

logic [4:0] mem[1900];


logic [23:0] pal[20];
//assign pal[0] = 24'hFF0000;
//assign pal[1] = 24'hF83800;
//assign pal[2] = 24'hF0D0B8;
//assign pal[3] = 24'h503000;
//assign pal[4] = 24'hFFE0A8;
//assign pal[5] = 24'h0058F8;
//assign pal[6] = 24'hFCFCFC;
//assign pal[7] = 24'hBCBCBC;
//assign pal[8] = 24'hA40000;
//assign pal[9] = 24'hD82800;
//assign pal[10] = 24'hFC7460;
//assign pal[11] = 24'hFCBCB0;
//assign pal[12] = 24'hF0BC3C;
//assign pal[13] = 24'hAEACAE;
//assign pal[14] = 24'h363301;
//assign pal[15] = 24'h6C6C01;
assign pal[0] = 24'h050505;
assign pal[1] = 24'h92817E;
assign pal[2] = 24'h4B352B;
assign pal[3] = 24'h7B6B58;
assign pal[4] = 24'h503615;
assign pal[5] = 24'hD3CDC9;
assign pal[6] = 24'h3C2002;
assign pal[7] = 24'h96876C;
assign pal[8] = 24'hB5AEA3;
assign pal[9] = 24'h584413;
assign pal[10] = 24'h553F0B;
assign pal[11] = 24'h6C5312;
assign pal[12] = 24'h60460C;
assign pal[13] = 24'hA08928;
assign pal[14] = 24'h5A400A;
assign pal[15] = 24'hBDA533;
assign pal[16] = 24'h9F9F6B;
assign pal[17] = 24'hB8B8A8;
assign pal[18] = 24'hD4D4AA;
assign pal[19] = 24'hABCDEF;
logic[4:0] index;

initial
begin
		$readmemh("./Right.txt",mem);
end

//logic[18:0] temp;
//assign temp = DrawX + DrawY*19'd640;
//logic[18:0] char_addr;
//assign char_addr = addr; //+ 19'd300800;
always_comb// @ (posedge Clk)
begin
	if(in_range)
	index =mem[addr];
	else if(DrawY > 410)
	index = 5'd10;
	else if(is_plane)
	index= mem[plane_addr+16'd1500];
	else if(is_missle)
	index = 5'd19;
	else
	index = mem[0];
end


assign data_out = pal[index];

endmodule
