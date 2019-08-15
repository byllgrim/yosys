/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/yosys.h"
#include "kernel/sigtools.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

#include "passes/pmgen/demo_reduce_pm.h"

void create_reduce(demo_reduce_pm &pm)
{
	auto &st = pm.st_reduce;
	auto &ud = pm.ud_reduce;

	if (ud.longest_chain.empty())
		return;

	log("Found chain of length %d (%s):\n", GetSize(ud.longest_chain), log_id(st.first->type));

	SigSpec A;
	SigSpec Y = ud.longest_chain.front().first->getPort(ID(Y));
	auto last_cell = ud.longest_chain.back().first;

	for (auto it : ud.longest_chain) {
		auto cell = it.first;
		if (cell == last_cell) {
			A.append(cell->getPort(ID(A)));
			A.append(cell->getPort(ID(B)));
		} else {
			A.append(cell->getPort(it.second == ID(A) ? ID(B) : ID(A)));
		}
		log("    %s\n", log_id(cell));
		pm.autoremove(cell);
	}

	Cell *c;

	if (last_cell->type == ID($_AND_))
		c = pm.module->addReduceAnd(NEW_ID, A, Y);
	else if (last_cell->type == ID($_OR_))
		c = pm.module->addReduceOr(NEW_ID, A, Y);
	else if (last_cell->type == ID($_XOR_))
		c = pm.module->addReduceXor(NEW_ID, A, Y);
	else
		log_abort();

	log("    -> %s (%s)\n", log_id(c), log_id(c->type));
}

struct DemoReducePass : public Pass {
	DemoReducePass() : Pass("demo_reduce", "map chains of AND/OR/XOR") { }
	void help() YS_OVERRIDE
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    demo_reduce [options] [selection]\n");
		log("\n");
		log("Demo for recursive pmgen patterns. Map chains of AND/OR/XOR to $reduce_*.\n");
		log("\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) YS_OVERRIDE
	{
		log_header(design, "Executing DEMO_REDUCE pass.\n");

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			// if (args[argidx] == "-singleton") {
			// 	singleton_mode = true;
			// 	continue;
			// }
			break;
		}
		extra_args(args, argidx, design);

		for (auto module : design->selected_modules())
			demo_reduce_pm(module, module->selected_cells()).run_reduce(create_reduce);
	}
} DemoReducePass;

PRIVATE_NAMESPACE_END