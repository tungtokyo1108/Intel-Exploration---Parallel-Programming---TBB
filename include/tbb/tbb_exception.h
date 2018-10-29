/*
 * tbb_exception.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_TBB_EXCEPTION_H_
#define INCLUDE_TBB_TBB_EXCEPTION_H_

#include "tbb_stddef.h"
#include <exception>
#include <new>
#include <string>

namespace tbb {

// Exception for cocurrent container
class bad_last_alloc : public std::bad_alloc {
public:
	const char* what() const throw() __TBB_override;
#if __TBB_DEFAULT_DTOR_THROW_SPEC_BROKEN
	~bad_last_alloc() throw() __TBB_override {}
#endif
};

// Exception for PPL locks
class improper_lock : public std::exception {
public:
	const char* what() const throw() __TBB_override;
};

class user_abort : public std::exception {
public:
	const char* what() const throw() __TBB_override;
};

class missing_wait : public std::exception {
public:
	const char* what() const throw() __TBB_override;
};

class invalid_multiple_sheduling : public std::exception {
public:
	const char* what() const throw() __TBB_override;
};

namespace internal {
void __TBB_EXPORTED_FUNC throw_bad_last_alloc_exception_v4();
enum exception_id {
	    eid_bad_alloc = 1,
	    eid_bad_last_alloc,
	    eid_nonpositive_step,
	    eid_out_of_range,
	    eid_segment_range_error,
	    eid_index_range_error,
	    eid_missing_wait,
	    eid_invalid_multiple_scheduling,
	    eid_improper_lock,
	    eid_possible_deadlock,
	    eid_operation_not_permitted,
	    eid_condvar_wait_failed,
	    eid_invalid_load_factor,
	    eid_reserved, // free slot for backward compatibility, can be reused.
	    eid_invalid_swap,
	    eid_reservation_length_error,
	    eid_invalid_key,
	    eid_user_abort,
	    eid_reserved1,
	#if __TBB_SUPPORTS_WORKERS_WAITING_IN_TERMINATE
	    eid_blocking_thread_join_impossible = eid_reserved1,
	#endif
	    eid_bad_tagged_msg_cast,
	    //! The last enumerator tracks the number of defined IDs. It must remain the last one.
	    /** When adding new IDs, place them immediately _before_ this comment (that is
	        _after_ all the existing IDs. NEVER insert new IDs between the existing ones. **/
	    eid_max
};

/*
 * Gathers all throw operators in one place
 * Its purpose is to minimize code bloat that can be caused by throw operators
 * scattered in multiple places, especially in templates
 */

void __TBB_EXPORTED_FUNC throw_exception_v4 (exception_id);
inline void throw_exception(exception_id eid) {throw_exception_v4(eid);}

}
}

#if __TBB_TASK_GROUP_CONTEXT
#include "tbb_allocator.h"
#include <typeinfo>

namespace tbb {
	/*
	* Interface to be implemented by all exceptions TBB recognizes and propagates across the threads.
	*
	* If an unhandled exception of the type derived from tbb::tbb_exception is interceptd by
	* the TBB schedule in one of the worker threads and it is delivered to and re-thrown in the root thread.
	* The root thread is the thread that has started the outermost algorithm or root tast sharing the same
	* task_group_context with the guilty algorithm
	*/

	class tbb_exception : public std::exception
	{
		void* operator new (size_t);
	public:
		#if __clang__
		~tbb_exception() throw() {}
		#endif

		/*Creates and return pointer to the deep copy of this exception object*/
		virtual tbb_exception* move() throw() = 0;
		/*Destroys objects created by the move() method
		* Free memory and calls destructor for this exception object. */
		virtual void destroy() throw() = 0;
		/* Throw this exception object
		* Make sure that if you several levels of derivation from this interface
		* you implement or override thsi method on the most derived level
		*/
		virtual void throw_self() = 0;
		// Return RTTI name of the originally intercepted exception
		virtual const char* name() const throw() = 0;
		// Return the result of originally intercepted exception what() method
		virtual const char* what() const throw() __TBB_override = 0;
		/* operator delete is provided only to allow using existing smart pointers
		* with TBB exception object obtained as the result of applying move() operation
		* on an exception throw out of TBB schedule
		*/
		void operator delete (void* p) {
			internal::deallocate_via_handler_v3(p);
		}
	};

	/* This class is used by TBB to propagate information about unhandled exceptions into the root thread
	* Exception of this type is thrown by TBB in the root thread
	* if an unhandled exception was intercepted during the algorithm execution in one the worker
	*/
	class captured_exception : public tbb_exception
	{
	public:
		captured_exception(const captured_exception& src)  : tbb_exception(src), my_dynamic(false)
		{
			set(src.my_exception_name, src.my_exception_info);
		}

		captured_exception(const char* name_, const char* info)
		{
			set(name_, info);
		}

		__TBB_EXPORTED_METHOD ~captured_exception() throw();

		captured_exception& operator= (const captured_exception& src) {
			if (this != &src) {
				clear();
				set(src.my_exception_name, src.my_exception_info);
			}
			return *this;
		}

		captured_exception* __TBB_EXPORTED_METHOD move() throw() __TBB_override;
		void __TBB_EXPORTED_METHOD destroy() throw() __TBB_override;
		void throw_self() __TBB_override {__TBB_THROW(*this);}
		const char* __TBB_EXPORTED_METHOD name() const throw() __TBB_override;
		const char* __TBB_EXPORTED_METHOD what() const throw() __TBB_override;
		void __TBB_EXPORTED_METHOD set(const char* name, const char* info) throw();
		void __TBB_EXPORTED_METHOD clear() throw();

	private:
		captured_exception() {}
		static captured_exception* allocate(const char* name, const char* info);
		bool my_dynamic;
		const char* my_exception_name;
		const char* my_exception_info;
	};

	/*Template that can be used to implement exception that tranfers arbitrary Exception to the root thread*/
	template <typename ExceptionData>
	class movable_exception : public tbb_exception
	{
		typedef movable_exception<ExceptionData> self_type;
	public:
		movable_exception(const ExceptionData& data_)
		      : my_exception_data(data_)
					, my_dynamic(false)
					, my_exception_name(
						#if TBB_USE_EXCEPTIONS
						typeid(self_type).name()
						#else
						"movable_exception"
						#endif
					)
					{}

		movable_exception(const movable_exception& src) throw()
		      : tbb_exception(src)
					, my_exception_data(src.my_exception_data)
					, my_dynamic(false)
					, my_exception_name(src.my_exception_name)
					{}

		~movable_exception() throw() {}
		const movable_exception& operator= (const movable_exception& src) {
			if (this != &src) {
				my_exception_data = src.my_exception_data;
				my_exception_name = src.my_exception_name;
			}
			return *this;
		}
		ExceptionData& data() throw() {return my_exception_data;}
		const ExceptionData& data() __TBB_override {return my_exception_data;}
		const char* name() const throw() __TBB_override {return my_exception_name;}
		const char* what() const throw() __TBB_override {return "tbb::movable_exception";}

		movable_exception* move() throw() __TBB_override {
			void* e = internal::allocate_via_handler_v3(sizeof(movable_exception));
			if (e) {
				::new (e) movable_exception(*this);
				((movable_exception*)e)->my_dynamic = true;
			}
			return (movable_exception*)e;
		}

		void destroy() throw() __TBB_override {
			__TBB_ASSERT ( my_dynamic, "Method destroy can be called only on dynamically allocated movable_exceptions" );
			if (my_dynamic)
			{
				this->movable_exception();
				internal::deallocate_via_handler_v3(this);
			}
		}
		void throw_self() __TBB_override {__TBB_THROW(*this);}

	protected:
		ExceptionData my_exception_data;

	private:
		bool my_dynamic;
		const char* my_exception_name;
	};

	#if !TBB_USE_CAPTURED_EXCEPTION
	namespace internal {
		// Exception container that preserves the exact copy of the original exception
		class tbb_exception_ptr{
			std::exception_ptr my_ptr;

		public:
			static tbb_exception_ptr* allocate();
			static tbb_exception_ptr* allocate(const tbb_exception& tag);
			static tbb_exception_ptr* allocate(captured_exception& src);

			void destroy() throw();
			void throw_self() {std::rethrow_exception(my_ptr);}

		private:
			tbb_exception_ptr(const std::exception_ptr& src) : my_ptr(src) {}
			tbb_exception_ptr(const captured_exception& src) :
			    #if __TBB_MAKE_EXCEPTION_PTR_PRESENT
					my_ptr(std::make_exception_ptr(src))
					#else
					my_ptr(std::copy_exception(src))
					#endif
			{}
		};
	}
	#endif
}
#endif

#endif /* INCLUDE_TBB_TBB_EXCEPTION_H_ */
