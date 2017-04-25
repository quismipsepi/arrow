# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

class TestMutableBuffer < Test::Unit::TestCase
  def setup
    @data = "Hello"
    @buffer = Arrow::MutableBuffer.new(@data)
  end

  def test_mutable?
    assert do
      @buffer.mutable?
    end
  end

  def test_mutable_data
    assert_equal(@data, @buffer.mutable_data.to_s)
  end

  def test_slice
    sliced_buffer = @buffer.slice(1, 3)
    assert_equal(@data[1, 3], sliced_buffer.data.to_s)
  end
end
