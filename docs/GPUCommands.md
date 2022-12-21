GPU Address: `0x20000000`    
```
Layout:
 0x20000000: String: 'EMBRGPU '
 0x20000008-0x2000001b: Command FIFO
 0x2000001c:  u32  : Flags
  0x1: Geometry Cache Overflow
  0x2: Texture Cache Overflow
  0x4: FIFO Full
  0x8: FIFO Empty

Commands:
 NOP [0x00]
  No Arguments

  Self-explanatory
 CLEAR_FIFO [0x01]
  No Arguments

  Clears the Command FIFO
 SET_FOG_INFO [0x02]
  u32 Fog Start
  u32 Fog End
  u32 Fog Color

  Changes the current state of the fog info
 SET_AMBIENT_LIGHT [0x03]
  u32 color
 SET_LIGHT_INFO [0x04]
  u32 id
  u32 color
  u32* pos[3]
  u32 step
 CLEAR [0x05]
  u32 color

  Clears the framebuffer with the given color.
 UPLOAD_GEOMETRY [0x10]
  u32 id
  u32 topology
    0: Triangles
    1: Lines
  u32 count
  Vertex* vertices

  --- STRUCT DECLARATION ---
  typedef struct {
        uint32_t posXY; // (Signed, 12-bit fractional, 3-bit integer)
        uint32_t posZ; // (Signed, 12-bit fractional, 3-bit integer)
        uint32_t color;
        uint32_t texCoords; // (Signed, 15-bit fractional, no integer)
  } Vertex;

  Constructs a 3-Dimensional Mesh using a list of vertices and saves it to the GPU's Geometry Cache.
 FREE_GEOMETRY [0x11]
  u32 id

  Frees the Mesh stored with this ID.
 RENDER_GEOMETRY [0x12]
  u32 id


 UPLOAD_TEXTURE [0x20]
 FREE_TEXTURE [0x21]
 BIND_TEXTURE [0x22]


 MATRIX_MODE [0x30]
  u32 matrixType
   0: Geometry Matrix
   1: View Matrix
   2: Projection Matrix

  Sets the current matrix that we're working with to the specified matrix type.
 MATRIX_IDENTIFY [0x31]
  No arguments

  Sets the matrix to the identity matrix
 MATRIX_IDENTIFY [0x32]
  No arguments

  Sets the matrix to the identity matrix
  
 SWAP_BUFFERS [0xff]
  No arguments

  Swaps GPU framebuffers, in other words, display the changes made.
```