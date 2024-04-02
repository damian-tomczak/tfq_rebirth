technique
{
  pass
  {
    ColorOp[0]   = SELECTARG1;
    ColorArg1[0] = DIFFUSE;
    AlphaOp[0]   = SELECTARG1;
    AlphaArg1[0] = DIFFUSE;
    CullMode = NONE;
    AlphaBlendEnable = true;
    SrcBlend = SRCALPHA;
    DestBlend = INVSRCALPHA;
    ZEnable = false;
  }
}
