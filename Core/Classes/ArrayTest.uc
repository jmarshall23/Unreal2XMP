//=============================================================================
// ArrayTest
// Tests all of the features of dynamic arrays currently known
// Also good if you're interested in how you can use dynamic arrays


class ArrayTest extends Commandlet;

var array<int> ImportArray;
var array<int> TestArray;
var array<ArrayTest> ObjArray;
var array<Vector> VectArray;
var array< struct InnerType { var int Dummy; var array<int> InnerArray; } > OuterArray;

function NestedTests()
{
	OuterArray[0].InnerArray[0] = 5; //acc'none times two
	assert(-1 != OuterArray[0].InnerArray[0]); //acc'none
	//shouldn't cause a crash...that's all I care about :)

	OuterArray.Length = 1;
	OuterArray[0].InnerArray[0] = 5;
	assert(OuterArray[0].InnerArray[0]==5);
}

function ObjectTests()
{
	local ArrayTest a;
	local vector somevect;
	local int i;
	
	a=new(self)class'ArrayTest';
	ObjArray[0]=a;
	assert(ObjArray[0]==a);
	ObjArray[0].TestArray[0]=1;
	assert(ObjArray[0].TestArray[0]==1);

	ObjArray.Remove(0,1);

	VectArray[0]=vect(1,1,1);
	assert(VectArray[0]==vect(1,1,1));
	somevect = vect(2,2,2);
	VectArray[1]=somevect;
	assert(VectArray[1]==vect(2,2,2));
	
	VectArray.length=3;
	VectArray[2].x=5;
	assert(VectArray[2] == vect(5,0,0));
	for( i=0; i<3; i++ )
		VectArray.Insert(i,1);

	for( i=0; i<VectArray.Length; i++ )
		if( VectArray[i] == vect(5,0,0) )
			VectArray.Remove(i,1);

	while( VectArray.Length > 0 )
		VectArray.Remove(0,1);
}

function RegularPassing(array<int> ABC)
{
	assert(ABC.length==6);
	assert(ABC[5]==5);
	assert(ABC[3]==15);
	ABC[3]=30;
	assert(ABC[3]==30);
}

function OutPassing(out array<int> ABC)
{
	assert(ABC[3]==15);
	ABC[3]=30;
	assert(ABC[3]==30);
}

function array<int> Returning()
{
	local array<int> NewArray;
	NewArray[3]=45;
	assert(NewArray[3]==45);
	return NewArray;
}

function ImportTests()
{
	assert(testarray.length==3);
	assert(testarray[0]==1);
	assert(testarray[1]==2);
	assert(testarray[2]==3);

	assert(importarray.length==4);
	assert(importarray[0]==1);
	assert(importarray[1]==2);
	assert(importarray[2]==3);
}

function RegularTests()
{

	testarray.length=200;
	assert(testarray.length==200);
	testarray[5] = 4;
	assert(testarray.length==200);
	assert(testarray[4]==0);
	assert(testarray[5]==4);
	assert(testarray[6]==0);

	testarray.length=2;
	assert(testarray.length==2);
	assert(testarray[5]==0); //acc'none
	assert(testarray.length==2);

	testarray[5]=5;
	assert(testarray[4]==0);
	assert(testarray[5]==5);
	assert(testarray[6]==0); //acc'none

	testarray[3]=15;
	assert(testarray[3]==15);
}

function InsertRemoveTests()
{
	testarray.length = 0;
	testarray.insert(0,5);
	assert(testarray.length==5);
	testarray[0] = 5;
	testarray.insert(0,5);
	assert(testarray.length==10);
	assert(testarray[0]==0);
	assert(testarray[5]==5);
	testarray.insert(0,0);

	testarray.remove(2,3);
	assert(testarray.length==7);
	assert(testarray[2]==5);
	testarray.remove(3,4);
	assert(testarray.length==3);
	assert(testarray[2]==5);
	assert(testarray[3]==0); //acc'none
	testarray.remove(0,testarray.length);
	assert(testarray.length==0);

	testarray.insert(0,5);
	testarray.remove(0,6);//warning
	assert(testarray.length==0);
	testarray.remove(0,1);//warning
	assert(testarray.length==0);
	testarray.length=1;
	testarray.remove(0,1);
	assert(testarray.length==0);
	testarray.remove(1,1);//warning
	assert(testarray.length==0);
	
}

event int Main( string Parms )
{
	local array<int> Copy;

	ImportTests();

	RegularTests();

	ObjectTests();

	RegularPassing(testarray);
	assert(testarray[3]==15);

	OutPassing(testarray);
	assert(testarray[3]==30);

	testarray = Returning();
	assert(testarray[3]==45);
	
	copy = testarray;
	testarray[0] = 25;
	assert(copy[0]!=25);

	InsertRemoveTests();

	NestedTests();
	
	return 0;
}

defaultproperties {
	TestArray(0)=1
	TestArray(1)=2
	TestArray(2)=3
	ImportArray=(1,2,3,4)

	HelpCmd="ArrayTest"
	HelpOneLiner="Tests various features of dynamic arrays"
	HelpUsage="ArrayTest (no parameters)"
	HelpWebLink=""
	LogToStdout=true
}
