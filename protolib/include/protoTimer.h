/*********************************************************************
 *
 * AUTHORIZATION TO USE AND DISTRIBUTE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: 
 *
 * (1) source code distributions retain this paragraph in its entirety, 
 *  
 * (2) distributions including binary code include this paragraph in
 *     its entirety in the documentation or other materials provided 
 *     with the distribution, and 
 *
 * (3) all advertising materials mentioning features or use of this 
 *     software display the following acknowledgment:
 * 
 *      "This product includes software written and developed 
 *       by Brian Adamson, Justin Dean, and Joe Macker of the Naval 
 *       Research Laboratory (NRL)." 
 *         
 *  The name of NRL, the name(s) of NRL  employee(s), or any entity
 *  of the United States Government may not be used to endorse or
 *  promote  products derived from this software, nor does the 
 *  inclusion of the NRL written and developed software  directly or
 *  indirectly suggest NRL or United States  Government endorsement
 *  of this product.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ********************************************************************/
 
 
#ifndef _PROTO_TIMER
#define _PROTO_TIMER

#include "protoDebug.h"
#include "protoDefs.h"  // for ProtoSystemTime()
#include "protoTime.h"
#include <math.h>

/**
 * @class ProtoTimer
 *
 * @brief This is a generic timer class which will notify a ProtoTimer::Listener upon timeout.
 *
 * A list of ProtoTimers is managed by the ProtoTimerMgr class after they are "activated".
 * The ProtoDispatcher's "main loop" calls ProtoTimerMgr::OnSystemTimeout at regular intervals
 * to determine if any timer objects have timed out.  If so, the callback function
 * associated with the ProtoTimer::Listener is invoked.  
 *
 */
class ProtoTimer
{
    friend class ProtoTimerMgr;
    
    public:
        ProtoTimer();
        ~ProtoTimer();
        
 		/**
		 * This function will define which method will be called upon a timer timing out
		 *
		 * TODO: Update text with new timer functionality
		 *
		 * IMPORTAN: Special notice should be taken of the boolean return value of this function.
		 * When returning "true" the number of repeats will be decremented and if below 0
		 * will deactivate the timer.  If, in this function, you are rescheduling the timer, deleting it,
		 * or repetions of the timer which invoked the call, the function should return "false"
		 * to avoid standard exit actions (i.e. the timeout function has overridden usual timer handling).

		 * NOTE: For VC++ 6.0 Debug builds, you _cannot_ use pre-compiled
         * headers with this template code.  Also, "/ZI" or "/Z7" compile options 
         * must NOT be specified.  (or else VC++ 6.0 experiences an "internal compiler error")
		 *
		 * @param theListener a pointer to the "listening" object
		 * @param timeoutHandler a pointer to the listener's callback function.
		 *
		 */
         
         
        // This is our new "ProtoTimer::SetListener()" where the listener timeoutHandler
        // has a "void" return type instead of the old "bool" since we don't care about
        // the return type anymore. (yay!) 
        template <class LTYPE>
        bool SetListener(LTYPE* theListener, void(LTYPE::*timeoutHandler)(ProtoTimer&))
        {
            if (NULL != listener) delete listener;
            listener = theListener ? new LISTENER_TYPE<LTYPE>(theListener, timeoutHandler) : NULL;
            return (NULL == listener) ? (NULL != theListener) : true;
        }
        
        //  This is the old "ProtoTimer::SetListener()" we keep for backwards-compatibility
        //  WARNING: This _will_ be deprecated in the future
		template <class LTYPE>
        bool SetListener(LTYPE* theListener, bool(LTYPE::*timeoutHandler)(ProtoTimer&))
        {
            if (NULL != listener) delete listener;
            listener = theListener ? new OLD_LISTENER_TYPE<LTYPE>(theListener, timeoutHandler) : NULL;
            return (NULL == listener) ? (NULL != theListener) : true;
        }
       
        /**
         * This method sets the interval (in seconds) after which the
         * timer will "fire" (i.e., the method specified by 
         * ProtoTimer::SetListener() is invoked).
         *
         * @param theInterval timer interval in seconds
         */ 
        // Our ProtoTime class currently only handles 1 usec granularity (for non-zero timeouts)
        // so we enforce that constraint here.
        void SetInterval(double theInterval) 
            {interval = (theInterval <= 0.0) ? 0.0 : ((theInterval < 1.0e-06) ? 1.0e-06 : theInterval);}
        double GetInterval() const {return interval;}
		/**
        * Timer repetition (0 =  one shot, -1 = infinite repetition)
        *
		* @param numRepeat number of timer repetitions
		*/
		void SetRepeat(int numRepeat) 
            {repeat = numRepeat;}
        int GetRepeat() const 
            {return repeat;}
        
        
        /// Provided for "advanced" timer monitoring/control
        void ResetRepeat() 
            {repeat_count = repeat;}
       /// Provided for "advanced" timer monitoring/control
  
		void DecrementRepeatCount() 
            { if (repeat_count > 0) repeat_count--; }
       /// Provided for "advanced" timer monitoring/control
  
		int GetRepeatCount() {return repeat_count;}
       /// Provided for "advanced" timer monitoring/control
  
		/// This must be used with wisdom!
        void SetRepeatCount(int repeatCount)
            {repeat_count = repeatCount;}
        
        //double GetTimeout() {return timeout;}
        
        // Activity status/control
        /**
		* Returns true if timer is associated with a ProtoTimerMgr
		*/
		bool IsActive() const {return (NULL != mgr);}
        double GetTimeRemaining() const;
        void Reset()
        {
            ResetRepeat();
            if (IsActive()) Reschedule();
        }
        bool Reschedule();
        void Scale(double factor);
        void Deactivate();
        
        // Ancillary
        void SetUserData(const void* userData) {user_data = userData;}
        const void* GetUserData() {return user_data;}
        
        /**
		* Installer commands
        */
		enum Command {INSTALL, MODIFY, REMOVE};
          
    private:
		/**
		* Invokes the "listener's" callback function.
		*
		* TODO: Update with new timer functionality.
		*
		* @retval Returns the result of the listener's callback function if
		* a listener exists for the timer.
		*/
        void DoTimeout() 
            {if (NULL !=  listener) listener->on_timeout(*this);}
            
		/**
		* @class Listener
		*
		* @brief Listener base class
		*/
        class Listener
        {
            public:
				/// virtual destructor
                virtual ~Listener() {}
                /// virtual on_timeout function
				virtual void on_timeout(ProtoTimer& theTimer) = 0;
        };
        /**
		* @class LISTENER_TYPE
		*
		* @brief Template for Listener classes. (This will be deprecated)
		*/
        template <class LTYPE>
        class LISTENER_TYPE : public Listener
        {
            public:
				/**
				* Listener constructor
				*
				* @param theListener pointer to the "listening" object
				* @param timeoutHandler *pointer to the Listener's callback function.
				*/
                LISTENER_TYPE(LTYPE* theListener, void(LTYPE::*timeoutHandler)(ProtoTimer&))
                    : listener(theListener), timeout_handler(timeoutHandler) {}
				/**
				* @retval Returns the result of the Listeners timeout handler.
                */
				void on_timeout(ProtoTimer& theTimer) 
                    {(listener->*timeout_handler)(theTimer);}
                /**
				* Duplicates the Listener member and returns a pointer to the new
				* object.
				*/
				Listener* duplicate()
                    {return (static_cast<Listener*>(new LISTENER_TYPE<LTYPE>(listener, timeout_handler)));}
            private:
                LTYPE* listener;
                void (LTYPE::*timeout_handler)(ProtoTimer&);
        };  // end class ProtoTimer::OLD_LISTENER_TYPE
        
		/**
		* @class OldListener
		*
		* @brief OldListener base class (will be deprecated)
		*/
        class OldListener : public Listener
        {
            public:
				/// virtual destructor
                virtual ~OldListener() {}
                /// virtual on_timeout function
				void on_timeout(ProtoTimer& theTimer)
                        {old_on_timeout(theTimer);}
            private:
                virtual bool old_on_timeout(ProtoTimer& theTimer) = 0;   
        };
        
        /**
		* @class OLD_LISTENER_TYPE
		*
		* @brief Template for Listener classes. (This will be deprecated)
		*/
        template <class LTYPE>
        class OLD_LISTENER_TYPE : public OldListener
        {
            public:
				/**
				* Listener contstructor
				*
				* @param theListener pointer to the "listening" object
				* @param timeoutHandler *pointer to the Listener's callback function.
				*/
                OLD_LISTENER_TYPE(LTYPE* theListener, bool(LTYPE::*timeoutHandler)(ProtoTimer&))
                    : listener(theListener), timeout_handler(timeoutHandler) {}
				/**
				* @retval Returns the result of the Listeners timeout handler.
                */
				bool old_on_timeout(ProtoTimer& theTimer) 
                    {return (listener->*timeout_handler)(theTimer);}
                /**
				* Duplicates the Listener member and returns a pointer to the new
				* object.
				*/
				OldListener* duplicate()
                    {return (static_cast<OldListener*>(new OLD_LISTENER_TYPE<LTYPE>(listener, timeout_handler)));}
            private:
                LTYPE* listener;
                bool (LTYPE::*timeout_handler)(ProtoTimer&);
        };  // end class ProtoTimer::OLD_LISTENER_TYPE
        
        Listener*                   listener;
        
        double                      interval;                
        int                         repeat;                  
        int                         repeat_count;            
        
        const void*                 user_data;
        ProtoTime                   timeout;                 
        bool                        is_precise;              
        class ProtoTimerMgr*        mgr;                     
        ProtoTimer*                 prev;                    
        ProtoTimer*                 next;                    
};  // end class ProtoTimer

/** 
 * @class ProtoTimerMgr
 *
 * @brief This class manages ProtoTimer instances when they are "activated". 
 * The ProtoDispatcher(see below) derives from this to manage ProtoTimers 
 * for an application. (The ProtoSimAgent base class contains a ProtoTimerMgr 
 * to similarly manage timers for a simulation instance). 
 */
class ProtoTimerMgr
{
    friend class ProtoTimer;
    
    public:
		/// Default constructor
        ProtoTimerMgr();
		/// Default destructor
        virtual ~ProtoTimerMgr();
        
        // ProtoTimer activation/deactivation
        virtual void ActivateTimer(ProtoTimer& theTimer);
        virtual void DeactivateTimer(ProtoTimer& theTimer);
        
        /**
        * @retval Returns "true" if there are any active timers
        */
        bool IsActive() const
            {return (NULL != short_head);}
        
        /**
		* @retval Returns any time remaining for the active short timer or -1
		*/
        double GetTimeRemaining() const 
            {return ((NULL != short_head) ? short_head->GetTimeRemaining() : -1.0);}
        
        /// Call this when the timer mgr's one-shot system timer fires
        void OnSystemTimeout();
        
        /// Special call to allow early dispatch of timeouts
        void DoSystemTimeout()
        {
            bool updateStatus = update_pending;
            update_pending = true;
            OnSystemTimeout();
            update_pending = updateStatus;
        }
        
        virtual void GetSystemTime(struct timeval& currentTime);
            
    protected:
        /// System timer association/management definitions and routines
        virtual bool UpdateSystemTimer(ProtoTimer::Command command,
                                       double              delay) = 0;// {return true;}
            
    private:
        // Methods used internally 
        void ReactivateTimer(ProtoTimer& theTimer, const ProtoTime& now);
        void InsertLongTimer(ProtoTimer& theTimer);
        bool InsertLongTimerReverse(ProtoTimer& theTimer);
        void RemoveLongTimer(ProtoTimer& theTimer);
        void InsertShortTimer(ProtoTimer& theTimer);
        bool InsertShortTimerReverse(ProtoTimer& theTimer);
        void RemoveShortTimer(ProtoTimer& theTimer);
        void Update();
        
        bool GetNextTimeout(ProtoTime& nextTimeout) const
        {
            if (NULL != short_head)
            {
                nextTimeout = short_head->timeout;
                return true;
            }
            else
            {
                return false;
            }
        }
        void GetPulseTime(ProtoTime& pulseTime) const
        {
            pulseTime = pulse_mark;
            pulseTime += (1.0 - pulse_timer.GetTimeRemaining());
        }
        bool OnPulseTimeout(ProtoTimer& theTimer);
        
        static const double PRECISION_TIME_THRESHOLD;
        
        
        void GetCurrentSystemTime(struct timeval& currentTime)
        {
#if  (defined(SIMULATE) && defined(NS2))
            GetSystemTime(currentTime);
#else
            ProtoSystemTime(currentTime);
#endif // if/else (SIMULATE && NS2)
        }
        
        // We need this to support some of our SIM code so it can
        // find its simulation context
        void GetCurrentProtoTime(ProtoTime& currentTime)
        {
#if  (defined(SIMULATE) && defined(NS2))
           GetSystemTime(currentTime.AccessTimeVal());
#else
           currentTime.GetCurrentTime();
#endif // if/else (SIMULATE && NS2)
        }
        
       
        // Member variables
        bool            update_pending;       
        bool            timeout_scheduled;                     
        ProtoTime       scheduled_timeout;                                   
        ProtoTimer      pulse_timer;  // one second pulse timer    
        ProtoTime       pulse_mark;                                
        ProtoTimer*     long_head;                                 
        ProtoTimer*     long_tail;                                 
        ProtoTimer*     short_head;                                
        ProtoTimer*     short_tail; 
        ProtoTimer*     invoked_timer;  // timer whose listener is being invoked
};  // end class ProtoTimerMgr

#endif // _PROTO_TIMER
