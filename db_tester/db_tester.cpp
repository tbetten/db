#include "db.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

/*
C:\Users\T. Betten\source\repos\db\db_tester>"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\bin\HostX86\x86"\link.exe /ERRORREPORT:PROMPT /OUT:"C:\Users\T. Betten\source\repos\db\Debug\db_tester.exe" /INCREMENTAL /NOLOGO /LIBPATH:"C:\Users\T. Betten\source\repos\db\Debug" /LIBPATH:"D:\vcpkg\installed\x86-windows\debug\lib" /LIBPATH:"D:\vcpkg\installed\x86-windows\debug\lib\manual-link" db.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib "D:\vcpkg\installed\x86-windows\debug\lib\*.lib" /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed /DEBUG:FASTLINK /PDB:"C:\Users\T. Betten\source\repos\db\Debug\db_tester.pdb" /SUBSYSTEM:CONSOLE /TLBID:1 /DYNAMICBASE /NXCOMPAT /IMPLIB:"C:\Users\T. Betten\source\repos\db\Debug\db_tester.lib" /MACHINE:X86 Debug\db_tester.obj
db.lib(db.obj) : error LNK2019: unresolved external symbol _sqlite3_close_v2 referenced in function "public: __thiscall db::DB_connection::DB_connection(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >,enum db::DB_connection::Mode)" (??0DB_connection@db@@QAE@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4Mode@01@@Z)
*/

int main()
{
	db::Blob b{};
	b.value = std::malloc(4);
	if (b.value) std::memset(b.value, 'A', 4);
	db::Blob c{};
	c.value = std::malloc(5);
	if (c.value) std::memset(c.value, 'B', 5);
	c.size = 5;

	b.size = 4;
	db::Blob b1{ b };
	if (!b.value) return 0;
	std::string s{ static_cast<char*> (b.value), 4 };
	std::cout << std::string{ static_cast<char*> (b.value), b.size } << "\n";
	std::cout << std::string{ static_cast<char*> (b1.value), b1.size } << "\n";
	b1 = c;
	std::cout << std::string{ static_cast<char*> (b1.value), b1.size } << "\n";
	db::Blob b2{ std::move(b1) };
	db::Blob b3{};
	b3 = std::move(b2);
	std::cout << std::string{ static_cast<char*> (b3.value), b3.size } << "\n";

	std::cout << fs::current_path() << "\n";
	//db::DB_connection db{ "db1.db" };
	auto db = db::DB_connection::create ("db1.db");

	auto stmt = db->prepare("select desc from test where code = ?");
	stmt.bind(1, 2);
	auto table = stmt.fetch_table();
	for (auto row : table)
	{
		auto val = std::get<std::string> (row["desc"]);
		std::cout << val << "\n";
	}

	//auto db1 = db::DB_factory::create ("db1.db", db::DB_connection::Mode::Read_write);
	auto db1 = db::DB_connection::create ("db1.db", db::DB_connection::Mode::Read_write);
	{
		//auto db2 = db::DB_factory::create ("db1.db");
		auto db2 = db::DB_connection::create ("db1.db");
	}
	//auto db3 = db::DB_factory::create ("db1.db");
	auto db3 = db::DB_connection::create ("db1.db");
	auto stmt1 = db3->prepare ("select desc from test where code = ?");
	stmt1.bind (1, 2);
	auto t = stmt1.fetch_table ();
	//auto db4 = db::DB_factory::create ("db1.db");
	auto db4 = db::DB_connection::create ("db1.db");
}