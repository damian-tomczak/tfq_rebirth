texture Texture1;
dword Color1;

technique
{
  pass
  {
    ColorOp[0]   = MODULATE;
    ColorArg1[0] = TEXTURE;
    ColorArg2[0] = TFACTOR;
    AlphaOp[0]   = MODULATE;
    AlphaArg1[0] = TEXTURE;
    AlphaArg2[0] = TFACTOR;
    ColorOp[1] = DISABLE;
    AlphaOp[1] = DISABLE;
    Texture[0] = (Texture1);
    MinFilter[0] = LINEAR;
    MagFilter[0] = LINEAR;
    CullMode = NONE;
    AlphaBlendEnable = true;
    SrcBlend = SRCALPHA;
    DestBlend = INVSRCALPHA;
	BlendOp = ADD;
    Lighting = false;
    TextureFactor = (Color1);
    ZEnable = false;
  }
}
