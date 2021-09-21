print( "Test" );

local value = TestReturn();
print("My value = " + value);

temp <- [ 1, 2, 3, 4 ];

foreach(idx,val in temp)
{
    print("index="+idx+" value="+val);
}

temp[0] = 33;
temp[1] = 34;
temp[2] = 35;
temp[3] = 36;

foreach(idx,val in temp)
{
    print("index="+idx+" value="+val);
}

foreach(idx,val in mytable)
{
	print("index="+idx+" value="+val);
}

print("x = " + mytable.controlpoint_1_vector.x);
print("y = " + mytable.controlpoint_1_vector.y);
print("z = " + mytable.controlpoint_1_vector.z);

local string = "controlpoint_" + 1;
local vectortable = string + "_vector";
local vector = Vector(0,0,0);
if ( vectortable in mytable )
{
	vector = Vector( mytable[vectortable].x, mytable[vectortable].y, mytable[vectortable].z );
}
print("Vector = " + vector);

local quaternion = Quaternion(343, 1337, 42, 69);
print("Quaternion = " + quaternion);

local matrix = matrix3x4_t();
local vecFwd = Vector(1, 2, 3);
local vecRight = Vector(4, 5, 6);
local vecUp = Vector(7, 8, 9);
local vecOrigin = Vector(21, 21, 21);

matrix = MatrixSetColumn(vecFwd, 0, matrix);
matrix = MatrixSetColumn(vecRight, 1, matrix);
matrix = MatrixSetColumn(vecUp, 2, matrix);
matrix = MatrixSetColumn(vecOrigin, 3, matrix);

print("Matrix = " + matrix);
print("MatrixAngle = " + MatrixAngles(matrix));
print("MatrixQuaternion = " + MatrixQuaternion(matrix));
