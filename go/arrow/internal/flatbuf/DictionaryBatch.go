// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Code generated by the FlatBuffers compiler. DO NOT EDIT.

package flatbuf

import (
	flatbuffers "github.com/google/flatbuffers/go"
)

/// For sending dictionary encoding information. Any Field can be
/// dictionary-encoded, but in this case none of its children may be
/// dictionary-encoded.
/// There is one vector / column per dictionary, but that vector / column
/// may be spread across multiple dictionary batches by using the isDelta
/// flag
type DictionaryBatch struct {
	_tab flatbuffers.Table
}

func GetRootAsDictionaryBatch(buf []byte, offset flatbuffers.UOffsetT) *DictionaryBatch {
	n := flatbuffers.GetUOffsetT(buf[offset:])
	x := &DictionaryBatch{}
	x.Init(buf, n+offset)
	return x
}

func (rcv *DictionaryBatch) Init(buf []byte, i flatbuffers.UOffsetT) {
	rcv._tab.Bytes = buf
	rcv._tab.Pos = i
}

func (rcv *DictionaryBatch) Table() flatbuffers.Table {
	return rcv._tab
}

func (rcv *DictionaryBatch) Id() int64 {
	o := flatbuffers.UOffsetT(rcv._tab.Offset(4))
	if o != 0 {
		return rcv._tab.GetInt64(o + rcv._tab.Pos)
	}
	return 0
}

func (rcv *DictionaryBatch) MutateId(n int64) bool {
	return rcv._tab.MutateInt64Slot(4, n)
}

func (rcv *DictionaryBatch) Data(obj *RecordBatch) *RecordBatch {
	o := flatbuffers.UOffsetT(rcv._tab.Offset(6))
	if o != 0 {
		x := rcv._tab.Indirect(o + rcv._tab.Pos)
		if obj == nil {
			obj = new(RecordBatch)
		}
		obj.Init(rcv._tab.Bytes, x)
		return obj
	}
	return nil
}

/// If isDelta is true the values in the dictionary are to be appended to a
/// dictionary with the indicated id
func (rcv *DictionaryBatch) IsDelta() byte {
	o := flatbuffers.UOffsetT(rcv._tab.Offset(8))
	if o != 0 {
		return rcv._tab.GetByte(o + rcv._tab.Pos)
	}
	return 0
}

/// If isDelta is true the values in the dictionary are to be appended to a
/// dictionary with the indicated id
func (rcv *DictionaryBatch) MutateIsDelta(n byte) bool {
	return rcv._tab.MutateByteSlot(8, n)
}

func DictionaryBatchStart(builder *flatbuffers.Builder) {
	builder.StartObject(3)
}
func DictionaryBatchAddId(builder *flatbuffers.Builder, id int64) {
	builder.PrependInt64Slot(0, id, 0)
}
func DictionaryBatchAddData(builder *flatbuffers.Builder, data flatbuffers.UOffsetT) {
	builder.PrependUOffsetTSlot(1, flatbuffers.UOffsetT(data), 0)
}
func DictionaryBatchAddIsDelta(builder *flatbuffers.Builder, isDelta byte) {
	builder.PrependByteSlot(2, isDelta, 0)
}
func DictionaryBatchEnd(builder *flatbuffers.Builder) flatbuffers.UOffsetT {
	return builder.EndObject()
}
