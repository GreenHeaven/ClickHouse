#pragma once

#include <Poco/SharedPtr.h>

#include <DB/Interpreters/ExpressionActions.h>
#include <DB/DataStreams/IProfilingBlockInputStream.h>


namespace DB
{

using Poco::SharedPtr;


/** Выполняет над блоком вычисление некоторого выражения.
  * Выражение состоит из идентификаторов столбцов из блока, констант, обычных функций.
  * Например: hits * 2 + 3, instr("yandex", url)
  * Выражение не меняет количество строк в потоке, и обрабатывает каждую строку независимо от других.
  */
class ExpressionBlockInputStream : public IProfilingBlockInputStream
{
public:
	ExpressionBlockInputStream(BlockInputStreamPtr input_, ExpressionActionsPtr expression_)
		: expression(expression_)
	{
		children.push_back(input_);
	}

	String getName() const { return "ExpressionBlockInputStream"; }

	String getID() const
	{
		std::stringstream res;
		res << "Expression(" << children.back()->getID() << ", " << expression->getID() << ")";
		return res.str();
	}

	const Block & getTotals()
	{
		if (IProfilingBlockInputStream * child = dynamic_cast<IProfilingBlockInputStream *>(&*children.back()))
		{
			totals = child->getTotals();

			if (totals)
				expression->execute(totals);
		}

		return totals;
	}

protected:
	Block readImpl()
	{
		Block res = children.back()->read();
		if (!res)
			return res;

		expression->execute(res);

		return res;
	}

private:
	ExpressionActionsPtr expression;
};

}
