
#include "db.h"
//#include "sqlite3.h"
#include <iostream>
#include <cstdlib>

using namespace std;
namespace db
{
	Blob::Blob(const Blob& other) noexcept
	{
		std::cout << "Blob copy constructor\n";
		value = std::malloc(other.size);
		if (value)
		{
			std::memcpy(value, other.value, other.size);
			size = other.size;
		}
		else
		{
			size = 0;
		}
	}

	Blob& Blob::operator=(const Blob& other) noexcept
	{
		std::cout << "Blob copy assignment\n";
		if (this == &other) return *this;
		if (size != other.size)
		{
			std::free(value);
			value = std::malloc(other.size);
			size = value ? other.size : 0;
		}
		std::memcpy(value, other.value, size);
		return *this;
	}

	Blob::Blob(Blob&& other) noexcept
	{
		std::cout << "Blob move constructor\n";
		value = other.value;
		size = other.size;
		other.value = nullptr;
		other.size = 0;
	}

	Blob& Blob::operator=(Blob&& other) noexcept
	{
		std::cout << "Blob move assignment\n";
		if (this == &other) return *this;
		std::free(value);
		value = other.value;
		size = other.size;
		other.value = nullptr;
		other.size = 0;
		return *this;
	}

	Blob::~Blob()
	{
		std::cout << "Blob destructor\n";
		std::free(value);
	}

	class Connection_deleter
	{
	public:
		void operator()(DB_connection* conn)
		{
			sqlite3_stmt* stmt = sqlite3_next_stmt (conn->m_connection, nullptr);
			while (stmt != nullptr)
			{
				sqlite3_finalize (stmt);
				stmt = sqlite3_next_stmt (conn->m_connection, stmt);
			}
			sqlite3_close_v2 (conn->m_connection);
		}
	};

	int convert_mode(DB_connection::Mode mode) noexcept
	{
		switch (mode)
		{
		case DB_connection::Mode::Create:
			return SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
		case DB_connection::Mode::Read_only:
			return SQLITE_OPEN_READONLY;
		case DB_connection::Mode::Read_write:
			return SQLITE_OPEN_READWRITE;
		default:
			return SQLITE_OPEN_READONLY;
		}
	}

	DB_connection::DB_connection(std::string path, Mode mode)// : m_path{ std::move(path) }, m_mode{ mode }
	{
		int rc;
		//rc = sqlite3_open_v2(m_path.c_str(), &m_connection, convert_mode(m_mode), nullptr);
		rc = sqlite3_open_v2 (path.c_str (), &m_connection, convert_mode (mode), nullptr);
		if (rc != SQLITE_OK)
		{
			std::string msg = sqlite3_errmsg(m_connection);
			sqlite3_close_v2(m_connection);
			m_connection = nullptr;
			throw (db_exception("Could not open database " + path + ": " + msg));
		}
	}

/*	DB_connection::DB_connection(const DB_connection& other) : DB_connection{ other.m_path, other.m_mode } {}

	DB_connection& DB_connection::operator=(DB_connection other)
	{
		swap(*this, other);
		return *this;
	}

	DB_connection::DB_connection(DB_connection&& other) noexcept
	{
		swap(*this, other);
	}

	DB_connection& DB_connection::operator=(DB_connection&& other) noexcept
	{
		if (this != &other)
		{
			close();
			swap(*this, other);
		}
		return *this;
	}

	DB_connection::~DB_connection() noexcept
	{
		std::cout << "connection destructor for " << m_path << "\n";
		close();
	}

	void DB_connection::close() noexcept
	{
		sqlite3_stmt* statement = sqlite3_next_stmt(m_connection, nullptr);
		while (statement != nullptr)
		{
			sqlite3_finalize(statement);
			statement = sqlite3_next_stmt(m_connection, statement);
		}
		sqlite3_close_v2(m_connection);
	}*/
	


	DB_connection::Ptr DB_connection::create (const std::string& name, Mode mode)
	{
		auto itr = std::find_if (std::begin (m_cache), std::end (m_cache), [&name, mode] (cache_entry& ce){return ce.key == name && ce.mode == mode; });
		if (itr == std::end (m_cache))
		{
			auto conn = std::shared_ptr<DB_connection> { new DB_connection{name, mode}, Connection_deleter{} };
			m_cache.emplace_back (name, mode, conn);
			return conn;
		}
		if (auto conn = itr->conn.lock ())
		{
			return conn;
		}
		else
		{
			auto new_conn = std::shared_ptr<DB_connection> (new DB_connection { name, mode }, Connection_deleter {});
			conn = new_conn;
			return new_conn;
		}
	}

	Prepared_statement DB_connection::prepare(std::string sql)
	{
		int rc;
		sqlite3_stmt* statement;
		//rc = sqlite3_prepare_v2(m_connection, sql.c_str(), -1, &statement, nullptr);
		rc = sqlite3_prepare_v2 (m_connection, sql.data(), -1, &statement, nullptr);
		if (rc == SQLITE_OK)
		{
			return Prepared_statement(statement);
		}
		else
		{
			std::string msg = sqlite3_errstr(rc);
			throw db_exception(msg);
		}
	}

/*	Prepared_statement::Prepared_statement(Prepared_statement&& other) noexcept
	{
		m_statement = other.m_statement;
		other.m_statement = nullptr;
	}

	Prepared_statement& Prepared_statement::operator=(Prepared_statement&& other) noexcept
	{
		sqlite3_finalize(m_statement);
		m_statement = other.m_statement;
		other.m_statement = nullptr;
		return *this;
	}

	Prepared_statement::~Prepared_statement() noexcept
	{
		sqlite3_finalize(m_statement);
	}*/

	Prepared_statement::Prepared_statement (sqlite3_stmt* stmt)
	{
		m_statement.reset (stmt);
	}

	void handle_error(int errorcode)
	{
		if (errorcode != SQLITE_OK)
		{
			throw db_exception(sqlite3_errstr(errorcode));
		}
	}

	template<class... Ts>
	struct overload : Ts...
	{
		using Ts::operator()...;
	};

	template<class... Ts>
	overload(Ts...)->overload<Ts...>;

	void Prepared_statement::bind(int index, value_t value)
	{
		std::visit
		(
			overload
			{
				[this, index](const std::monostate m) {handle_error(sqlite3_bind_null(m_statement.get(), index)); },
				[this, index](const int i) {handle_error(sqlite3_bind_int(m_statement.get(), index, i)); },
				[this, index](const double d) {handle_error(sqlite3_bind_double(m_statement.get(), index, d)); },
				[this, index](const std::string& s) {handle_error(sqlite3_bind_text(m_statement.get(), index, s.c_str(), -1, SQLITE_TRANSIENT)); },
				[this, index](const Blob b) {handle_error(sqlite3_bind_blob(m_statement.get(), index, b.value, b.size, SQLITE_TRANSIENT)); }
			}
			, value
		);
	}

	Prepared_statement::Row_result Prepared_statement::execute_row()
	{
		int rc = sqlite3_step(m_statement.get());
		if (rc == SQLITE_DONE)
		{
			return Row_result::Success;
		}
		if (rc == SQLITE_ROW)
		{
			return Row_result::Row;
		}
		throw db_exception(sqlite3_errstr(rc));
		return Row_result::Error;
	}

	row_t Prepared_statement::fetch_row()
	{
		auto ptr = m_statement.get ();
		auto num_columns = sqlite3_column_count(ptr);
		row_t result;
		for (int i = 0; i < num_columns; ++i)
		{
			std::string colname = sqlite3_column_name(ptr, i);
			auto coltype = sqlite3_column_type(ptr, i);
			switch (coltype)
			{
			case SQLITE_NULL:
				result[colname] = std::monostate{};
				break;
			case SQLITE_INTEGER:
				result[colname] = sqlite3_column_int(ptr, i);
				break;
			case SQLITE_FLOAT:
				result[colname] = sqlite3_column_double(ptr, i);
				break;
			case SQLITE_TEXT:
				result[colname] = std::string{ reinterpret_cast<const char*> (sqlite3_column_text(ptr, i)) };
				break;
			case SQLITE_BLOB:
			{
				Blob b;
				b.value = const_cast<void*> (sqlite3_column_blob(ptr, i));
				b.size = sqlite3_column_bytes(ptr, i);
				result[colname] = b;
			}
			break;
			default:
				result[colname] = std::monostate{};
			}
		}
		return result;
	}

	table_t Prepared_statement::fetch_table()
	{
		table_t result;
		while (execute_row() == Row_result::Row)
		{
			result.push_back(fetch_row());
		}
		reset();
		return result;
	}

	void Prepared_statement::reset() noexcept
	{
		sqlite3_reset(m_statement.get());
		sqlite3_clear_bindings(m_statement.get());
	}


/*
	db_connection_ptr DB_factory::create (const std::string& db_name, db::DB_connection::Mode mode)
	{
		auto itr = std::find_if (std::begin (m_cache), std::end (m_cache), [&db_name, mode] (cache_entry& ce){return ce.key == db_name && ce.mode == mode; });
		if (itr == std::end (m_cache))
		{
			auto ptr = std::make_shared<DB_connection> (db_name, mode);
			//m_cache.emplace_back (cache_entry { db_name, mode, ptr });
			m_cache.emplace_back (db_name, mode, ptr);
			return ptr;
		}
		if (auto ptr = itr->conn.lock ())
		{
			return ptr;
		}
		else
		{
			auto p = std::make_shared<DB_connection> (db_name, mode);
			itr->conn = p;
			return p;
		}
	}

	std::vector<cache_entry> DB_factory::m_cache {};*/
}