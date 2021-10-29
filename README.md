                                                                                           
                    0      1      2      3                                                 
                +-----------------------------+ -+                                         
              0 |            SIZE             |  |                                         
                ---------------+-------+-------  |  Base Header (8) bytes                  
              4 |   OPERATION  |  OFF  | FLAG |  |                                         
                ---------------+-------+------- -|                                         
         FROM 8 |          EXTENSIONS         |  |  Extension Header (OFF * 4 - 8) bytes   
                ------------------------------- -|                                         
     0 + OFF * 4|                             |  |                                         
                |                             |  |                                         
                |                             |  |                                         
                |                             |  |                                         
                |            DATA             |  |  Payload/Body (SIZE - OFF * 4) bytes    
                |                             |  |                                         
                |                             |  |                                         
                |                             |  |                                         
                |                             |  |                                         
                |                             |  |                                         
                +-----------------------------+ -+                                          
