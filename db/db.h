#pragma once  
#include <memory>
#include <string>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <variant>
#include <span>
#include "sqlite3.h"

namespace db
{
	using namespace std::string_literals;
	struct Blob
	{
		Blob() : size{ 0 }, value{ nullptr }{}
		Blob(const Blob& other) noexcept;
		Blob& operator=(const Blob& other) noexcept;
		Blob(Blob&& other) noexcept;
		Blob& operator=(Blob&& other) noexcept;
		~Blob() noexcept;

		size_t size;
		void* value;
	};

	class Prepared_statement;
	class Connection_deleter;

	class DB_connection
	{
		friend class Connection_deleter;
	public:
		enum class Mode : int { Read_only, Read_write, Create };
		using Ptr = std::shared_ptr<DB_connection>;

		static Ptr create (const std::string& name, Mode mode = Mode::Read_only);
		Prepared_statement prepare(std::string_view sql);
	private:
		struct cache_entry
		{
			cache_entry (std::string k, DB_connection::Mode m, std::weak_ptr<DB_connection> p) : key { std::move (k) }, mode { m }, conn { p }{}
			std::string key;
			DB_connection::Mode mode;
			std::weak_ptr<DB_connection> conn;
		};
		DB_connection (std::string name, Mode mode);
		inline static std::vector<cache_entry> m_cache {};
		sqlite3* m_connection;
	};

	class Statement_deleter
	{
	public:
		void operator()(sqlite3_stmt* stmt)
		{
			sqlite3_finalize (stmt);
		}
	};

	using value_t = std::variant<std::monostate, int, double, std::string, Blob>;
	using row_t = std::unordered_map<std::string, value_t>;
	using table_t = std::vector<row_t>;

	class Prepared_statement
	{
	public:
		enum class Row_result { Success, Row, Error };
		explicit Prepared_statement (sqlite3_stmt* stmt);

		void bind(int index, value_t value);
		void bind(std::span<value_t> values);
		Row_result execute_row();
		row_t fetch_row();
		table_t fetch_table();
		void reset() noexcept;
	private:
		std::unique_ptr<sqlite3_stmt, Statement_deleter> m_statement;
	};

	using db_connection_ptr = std::shared_ptr<DB_connection>;

	class db_exception : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	bool as_bool(value_t value);
	int as_int(value_t value);
	std::string as_string(value_t value);
	double as_double(value_t value);
	bool is_null(value_t value);
}