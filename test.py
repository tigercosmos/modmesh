import _modmesh as modmesh

arrayplex_int32 = modmesh.SimpleArray((2, 3, 4), dtype="int32")
modmesh.testhelper.TestSimpleArrayHelper.test_cast_int32_array(arrayplex_int32)