#pragma once  
#include <memory>
#include <string>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <variant>

struct sqlite3_stmt;
struct sqlite3;

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
	class DB_connection
	{
	public:
		enum class Mode { Read_only, Read_write, Create };

		DB_connection() : m_path{ ""s }, m_connection{ nullptr }, m_mode{ Mode::Read_only } {}
		explicit DB_connection(std::string path, Mode mode = Mode::Read_only);
		DB_connection(const DB_connection& other);
		DB_connection& operator= (DB_connection other);
		DB_connection(DB_connection&& other) noexcept;
		DB_connection& operator= (DB_connection&& other) noexcept;
		~DB_connection() noexcept;
		friend inline void swap(DB_connection& first, DB_connection& second) noexcept
		{
			using std::swap;
			swap(first.m_path, second.m_path);
			swap(first.m_mode, second.m_mode);
			swap(first.m_connection, second.m_connection);
		}
		Prepared_statement prepare(std::string sql);
	private:
		void close() noexcept;

		std::string m_path;
		Mode m_mode;
		sqlite3* m_connection;
	};

	using value_t = std::variant<std::monostate, int, double, std::string, Blob>;
	using row_t = std::unordered_map<std::string, value_t>;
	using table_t = std::vector<row_t>;

	class Prepared_statement
	{
	public:
		enum class Row_result { Success, Row, Error };

		Prepared_statement() : m_statement{ nullptr } {}
		explicit Prepared_statement(sqlite3_stmt* stmt) : m_statement{ stmt } {}
		Prepared_statement(const Prepared_statement& other) = delete;
		Prepared_statement& operator=(Prepared_statement& other) = delete;
		Prepared_statement(Prepared_statement&& other) noexcept;
		Prepared_statement& operator=(Prepared_statement&& other) noexcept;
		~Prepared_statement() noexcept;

		void bind(int index, value_t value);
		Row_result execute_row();
		row_t fetch_row();
		table_t fetch_table();
		void reset() noexcept;
	private:
		sqlite3_stmt* m_statement;
	};

	using db_connection_ptr = std::shared_ptr<DB_connection>;
	//using value_t = std::variant<std::monostate, int, double, std::string>;

	class db_exception : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};


	struct cache_entry;

	class DB_factory
	{
	public:
		static db_connection_ptr create (const std::string& db_name, DB_connection::Mode mode = DB_connection::Mode::Read_only);
	private:
		static std::vector<cache_entry> m_cache;
	};

}