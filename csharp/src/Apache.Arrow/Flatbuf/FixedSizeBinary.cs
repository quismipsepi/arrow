// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

namespace Apache.Arrow.Flatbuf
{

using global::System;
using global::FlatBuffers;

public struct FixedSizeBinary : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static FixedSizeBinary GetRootAsFixedSizeBinary(ByteBuffer _bb) { return GetRootAsFixedSizeBinary(_bb, new FixedSizeBinary()); }
  public static FixedSizeBinary GetRootAsFixedSizeBinary(ByteBuffer _bb, FixedSizeBinary obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p.bb_pos = _i; __p.bb = _bb; }
  public FixedSizeBinary __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  /// Number of bytes per value
  public int ByteWidth { get { int o = __p.__offset(4); return o != 0 ? __p.bb.GetInt(o + __p.bb_pos) : (int)0; } }

  public static Offset<FixedSizeBinary> CreateFixedSizeBinary(FlatBufferBuilder builder,
      int byteWidth = 0) {
    builder.StartObject(1);
    FixedSizeBinary.AddByteWidth(builder, byteWidth);
    return FixedSizeBinary.EndFixedSizeBinary(builder);
  }

  public static void StartFixedSizeBinary(FlatBufferBuilder builder) { builder.StartObject(1); }
  public static void AddByteWidth(FlatBufferBuilder builder, int byteWidth) { builder.AddInt(0, byteWidth, 0); }
  public static Offset<FixedSizeBinary> EndFixedSizeBinary(FlatBufferBuilder builder) {
    int o = builder.EndObject();
    return new Offset<FixedSizeBinary>(o);
  }
};


}
