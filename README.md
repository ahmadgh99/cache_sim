# cache_sim
2-Level cache simulator with write back and the option to choose between no/write allocate

run command: ./cacheSim <input file> --mem-cyc <# of cycles> --bsize <block log2(size)>
     --wr-alloc <0: No Write Allocate; 1: Write Allocate>
     --l1-size <log2(size)> --l1-assoc <log2(# of ways)> --l1-cyc <# of cycles>
     --l2-size <log2(size)> --l2-assoc <log2(# of ways)> --l2-cyc <# of cycles>

input file must include this format: <r:read/w:write> <addres in hexadecimal>
outputs: <L1 missrate>  <L2 missrate>  <average access time> 
