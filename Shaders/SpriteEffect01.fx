texture Texture;

technique
{
  pass
  {
    ColorOp[0]   = MODULATE;
    ColorArg1[0] = DIFFUSE;
    ColorArg2[0] = TEXTURE;
    AlphaOp[0]   = MODULATE;
    AlphaArg1[0] = DIFFUSE;
    AlphaArg2[0] = TEXTURE;
    Texture[0] = (Texture);
    MinFilter[0] = LINEAR;
    MagFilter[0] = LINEAR;
    CullMode = NONE;
    AlphaBlendEnable = true;
    SrcBlend = SRCALPHA;
    DestBlend = INVSRCALPHA;
    ZEnable = false;
  }
}
