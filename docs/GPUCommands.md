GPU Address: `0x20000000`    
```
Layout:
 0x2000_0000: String: 'EMBRGPU '
 0x2000_0008-0x2000_001b: Command FIFO
 0x2000_001c:  u32  : FIFO Ready (0 if full)

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
  
 SET_LIGHT_INFO [0x04]
  u32 id
  u32 color
  u32* pos[3]
  u32 step
 CLEAR [0x05]
  u32 color

  Clears the framebuffer with the given color.
 SET_CAMERA [0x06]
 UPLOAD_GEOMETRY [0x07]
 UPLOAD_GEOMETRY_LIGHT [0x08]
 UPLOAD_GEOMETRY_FOG [0x09]
 UPLOAD_GEOMETRY_LIGHT_FOG [0x0a]
  u32 topology
    0: Triangles
    1: Lines
  u32 count
  Vertex* verticies

  --- STRUCT DECLARATION ---
  typedef struct {
        uint32_t posXY;
        uint32_t posZ;
        uint32_t color;
        uint32_t texCoordX;
        uint32_t texCoordY;
  } Vertex;

  Constructs a 3-Dimensional Mesh and Rasterizes it onto the Framebuffer
  Will also apply fog and lighting if enabled to do such with the mesh
 UPLOAD_TEXTURE [0x0b]
 FREE_TEXTURE [0x0c]
 BIND_TEXTURE [0x0d]
```