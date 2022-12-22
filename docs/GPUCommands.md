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
 0x20000020:  u32  : Frame Count

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
  u32 posXY // (Signed, 15-bit fractional, divides by 128)
  u32 posZStep // (Signed, 15-bit fractional, divides by 128) (Step is a 16-bit unsigned integer and is not a float) 
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
        uint32_t posXY;   // (Signed, 15-bit fractional, divides by 256)
        uint32_t posZ;    // (Signed, 15-bit fractional, divides by 256)
        uint32_t color;
        uint32_t texCoords; // (Signed, 15-bit fractional, divides by 16384, fractional value beyond 0.0-1.0 (0x4000) is prohibited)
  } Vertex;

  Constructs a 3-Dimensional Mesh using a list of vertices and saves it to the GPU's Geometry Cache.
 FREE_GEOMETRY [0x11]
  u32 id

  Frees the Mesh stored with this ID.
 RENDER_GEOMETRY [0x12]
  u32 id

  Renders the Mesh stored at the given ID onto the screen (GPU Matrices are also applied as well.)
 UPLOAD_TEXTURE [0x20]
  u32 formatAndId
   0: 16 Color Indexed
   1: 256 Color Indexed
   2: 15-bit color with 1-bit alpha
  u32 size
  void* data
  void* palette (Can be a null pointer if format is 15-bit color)
 FREE_TEXTURE [0x21]
 BIND_TEXTURE [0x22]


 MATRIX_MODE [0x30]
  u32 matrixType
   0: Projection Matrix
   1: Geometry Matrix
   2: View Matrix

  Sets the current matrix that we're working with to the specified matrix type.
 MATRIX_IDENTITY [0x31]
  No arguments

  Sets the matrix to the identity matrix
 MATRIX_PERSPECTIVE [0x32]
  u32 nearPlane
  u32 farPlane
  u32 fov

  Calculates the perspective matrix and sets the current active matrix to the result.
 MATRIX_TRANSLATE [0x33]
  u32 posXY // (Signed, 15-bit fractional, divides by 128)
  u32 posZ  // (Signed, 15-bit fractional, divides by 128)

  Translates the current matrix to the given vector.
 MATRIX_ROTATE [0x34]
  u32 rotXY // (Signed, 15-bit fractional, divides by 32768)
  u32 rotZ  // (Signed, 15-bit fractional, divides by 32768)

  Rotates the current matrix to the given value. (Note: 8192 = 90 degress, 16384 = 180 degrees, just though I note that ;3)
 MATRIX_SCALE [0x35]
  u32 sclXY // (Signed, 15-bit fractional, divides by 128)
  u32 sclZ  // (Signed, 15-bit fractional, divides by 128)

  Scales the current matrix to the given value.
  
 SWAP_BUFFERS [0xff]
  No arguments

  Swaps GPU framebuffers, in other words, display the changes made.
```