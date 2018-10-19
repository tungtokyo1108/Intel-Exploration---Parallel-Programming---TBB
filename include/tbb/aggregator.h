/*
 * aggregator.h
 *
 *  Created on: Oct 15, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_AGGREGATOR_H_
#define INCLUDE_TBB_AGGREGATOR_H_

#if !TBB_PREVIEW_AGGREGATOR
#error Set TBB_PREVIEW_AGGREGATOR before including aggregator.h
#endif

#include "atomic.h"
#include "tbb_profiling.h"

namespace tbb {
namespace interface6 {

using namespace tbb::internal;
class aggregator_operation {
	template<typename handler_type> friend class aggregator_ext;
	uintptr_t status;
	aggregator_operation* my_next;
public:
	enum aggregator_operation_status {agg_waiting = 0, agg_finished};
	aggregator_operation() : status(agg_waiting), my_next(NULL) {}
	void start() {call_itt_notify(acquired, &status);}
	void finish() {itt_store_word_with_release(status, uintptr_t(agg_finished));}
	aggregator_operation *next() {return itt_hide_load_word(my_next);}
	void set_next(aggregator_operation* n) {itt_hide_store_word(my_next,n);}
};

namespace internal {

class basic_operation_base : public aggregator_operation {
	friend class basic_handler;
	virtual void apply_body() = 0;
public:
	basic_operation_base() : aggregator_operation() {}
	virtual ~basic_operation_base() {}
};

template<typename Body>
class basic_operation : public basic_operation_base, no_assign {
	const Body& my_body;
	void apply_body() __TBB_override {my_body();}
public:
	basic_operation(const Body& b) : basic_operation_base(), my_body(b) {}
};

class basic_handler {
public:
	basic_handler() {};
	void operator() (aggregator_operation* op_list) const {
		while (op_list)
		{
			/*
			 * op_list->status tag is used to cover accesses to the operation data.
			 *
			 * The executing thread acquires the tag and then perform the associated operation
			 * w/o triggering a race condition diagnostics
			 *
			 * A thread that created the operation is waiting for its status,
			 * so when this thread is done with the operation, it will release the tag
			 * and update the status to give control back to the waiting thread.
			 */
			basic_operation_base& request = static_cast<basic_operation_base&>(*op_list);
			op_list = op_list->next();
			request.start();
			request.apply_body();
			request.finish();
		}
	}
};
}

/*
 * An aggregator for collecting operations coming from multiple sources
 * and executing them executing them serially on a single thread
 */

template <typename handler_type>
class aggregator_ext : tbb::internal::no_copy {
public:
	aggregator_ext(const handler_type& h) : handler_busy(0), handle_operations(h) {mailbox = NULL;}
	void process(aggregator_operation *op) {execute_impl(*op);}

protected:
	/*
	 * Place operation in mailbox, then either handle mailbox
	 * or wait for the operation to be completed by a different thread
	 */
	void execute_impl(aggregator_operation& op) {
		aggregator_operation *res;

		/*
		 * op.status tag is used to cover accesses to this operation.
		 * This thread has created the operation,
		 * and now releases it so that the handler thread may handle
		 * the associated operation w/o triggering a race condition
		 */

		call_itt_notify(releasing, &(op.status));
		do {
			op.my_next = res = mailbox;
		} while (mailbox.compare_and_swap(&op,res) != res);
		if (!res)
		{
			/*
			 * &mailbox tag covers access to the handler_busy flag,
			 * which this waiting handler thread will try to set before entering handle operation
			 */
			call_itt_notify(acquired,&mailbox);
			start_handle_operations();
			__TBB_ASSERT(op.status,NULL);
		}
		else
		{
			call_itt_notify(prepare, &(op.status));
			spin_wait_while_eq(op.status, uintptr_t(aggregator_operation::agg_waiting));
			itt_load_word_with_acquire(op.status);
		}
	}

private:
	atomic<aggregator_operation *> mailbox;

	/*
	 * Controls thread access to handle_operations
	 * Behaves as boolean flag where 0 = flags, 1 = true;
	 */
	uintptr_t handler_busy;
	handler_type handle_operations;

	/*
	 * Trigger the handing of operations when the handler is free
	 */
	void start_handle_operations() {
		aggregator_operation *pending_operations;

		/*
		 * handler_busy tag covers access to mailbox as it is passed between active and waiting handlers.
		 *
		 * The waiting handler waits until the active handler releases and the waiting handler acquires
		 * &handler_busy as it becomes the active_handler.
		 *
		 * The release point is at the end of this function, when all operations is mailbox have been
		 * handled by the owner of this aggregator.
		 */
		call_itt_notify(prepare,&handler_busy);
		spin_wait_until_eq(handler_busy,uintptr_t(0));
		call_itt_notify(acquired, &handler_busy);

		// acquire fence not necessary here due to causality rule and surrounding atomics
		__TBB_store_with_release(handler_busy,uintptr_t(1));

		/*
		 * mail_box tag covers access to the handler_busy flags itself
		 * Capturing the state of the mailbox signifies that handler_busy has been
		 * set and a new active handler will now process that list is operations
		 */
		call_itt_notify(releasing, &mailbox);
		pending_operations = mailbox.fetch_and_add(NULL);
		handle_operations(pending_operations);
		itt_store_word_with_release(handler_busy,uintptr_t(0));
	}
};

class aggregator : private aggregator_ext<internal::basic_handler> {
public:
	aggregator() : aggregator_ext<internal::basic_handler>(internal::basic_handler()) {}
	/*
	 * The calling thread stores the function object in a basic_operation and
	 * places the operation in the aggregator's mailbox
	 */
	template<typename Body>
	void execute(const Body& b) {
		internal::basic_operation<Body> op(b);
		this->execute_impl(op);
	}
};
}

using interface6::aggregator;
using interface6::aggregator_ext;
using interface6::aggregator_operation;

}

#endif /* INCLUDE_TBB_AGGREGATOR_H_ */
