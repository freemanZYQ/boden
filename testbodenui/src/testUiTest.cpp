#include <bdn/init.h>
#include <bdn/test.h>

#include <bdn/mainThread.h>

#include <chrono>

using namespace bdn;


struct TestData : public Base
{
    int callCount = 0;
};



template<typename FuncType>
void testContinueSectionWith( FuncType scheduleContinueWith )
{
    // we verify that CONTINUE_SECTION_AFTER_PENDING_EVENTS works as expected

    P<TestData> pData = newObj<TestData>();

    SECTION("notCalledImmediately")
    {
        scheduleContinueWith(
            [pData]()
            {
                pData->callCount++;            
            } );        

        // should not have been called yet
        REQUIRE( pData->callCount==0 );
    }


    SECTION("notCalledBeforeExitingInitialFunction")
    {
        scheduleContinueWith(
            [pData]()
            {
                pData->callCount++;            
            } );        

        // even if we wait a while, the continuation should not be called yet
        // (not even if it runs in another thread).
        Thread::sleepMillis(2000);
        REQUIRE( pData->callCount==0 );
    }


    static P<TestData> pCalledBeforeNextSectionData;
    SECTION("calledBeforeNextSection-a")
    {
        pCalledBeforeNextSectionData = pData;

        scheduleContinueWith(
            [pData]()
            {
                pData->callCount++;            
            } );        
    }

    SECTION("calledBeforeNextSection-b")
    {
        REQUIRE( pCalledBeforeNextSectionData!=nullptr );

        // the continuation of the previous section should have been called

        REQUIRE( pCalledBeforeNextSectionData->callCount==1 );
    }

    SECTION("notCalledMultipleTimes")
    {        
        scheduleContinueWith(
            [pData]()
            {
                pData->callCount++;            

                REQUIRE( pData->callCount==1 );
            } );
    }


    static int subSectionInContinuationMask=0;
    SECTION("subSectionInContinuation-a")
    {
        scheduleContinueWith(
            [scheduleContinueWith]()
            {
                subSectionInContinuationMask |= 1;

                SECTION("a")
                {
                    SECTION("a1")
                    {
                        subSectionInContinuationMask |= 2;
                    }

                    SECTION("a2")
                    {
                        subSectionInContinuationMask |= 4;
                    }
                }

                // add another continuation
                SECTION("b")
                {
                    scheduleContinueWith(
                        []()
                        {
                            subSectionInContinuationMask |= 8;

                            SECTION("b1")
                            {
                                subSectionInContinuationMask |= 16;
                            }

                            SECTION("b2")
                            {
                                subSectionInContinuationMask |= 32;
                            }
                        } );
                }
            });
    }

    SECTION("subSectionInContinuation-b")
    {
        REQUIRE( subSectionInContinuationMask==63 );
    }

}

void testContinueSectionWith_expectedFail( void (*scheduleContinueWith)(std::function<void()>) )
{    
    SECTION("exceptionInContinuation")
    {
        scheduleContinueWith(
            []()
            {
                throw std::runtime_error("dummy error");
            } );        
    }

    SECTION("exceptionAfterContinuationScheduled")
    {
        scheduleContinueWith(
            []()
            {                
            } );        

        throw std::runtime_error("dummy error");
    }

    SECTION("failAfterContinuationScheduled")
    {
        scheduleContinueWith(
            []()
            {
            } );        

        REQUIRE(false);
    }
}


void scheduleContinueAfterPendingEventsWith( std::function<void()> continuationFunc )
{
    CONTINUE_SECTION_AFTER_PENDING_EVENTS_WITH(
        [continuationFunc]()
        {
            REQUIRE( Thread::isCurrentMain() );
            continuationFunc();
        } );    
}

TEST_CASE("CONTINUE_SECTION_AFTER_PENDING_EVENTS_WITH")
{
    testContinueSectionWith( scheduleContinueAfterPendingEventsWith );
}


TEST_CASE("CONTINUE_SECTION_AFTER_PENDING_EVENTS_WITH-expectedFail", "[!shouldfail]")
{
    testContinueSectionWith_expectedFail( scheduleContinueAfterPendingEventsWith );
}

TEST_CASE("CONTINUE_SECTION_AFTER_PENDING_EVENTS_WITH-asyncAfterSectionThatHadAsyncContinuation" )
{
	bool enteredSection = false;

    SECTION("initialChild")
    {
		enteredSection = true;
        CONTINUE_SECTION_AFTER_PENDING_EVENTS_WITH( [](){} );
    }

    std::function<void()> continuation =
        []()
        {
            SECTION("asyncChild1")
            {
            }

            SECTION("asyncChild2")
            {
            }
        };


	if(enteredSection)
	{
		// we should get a programmingerror here. It is not allowed to schedule a 
		// continuation when one was already scheduled
		REQUIRE_THROWS_PROGRAMMING_ERROR( CONTINUE_SECTION_AFTER_PENDING_EVENTS_WITH(continuation) );
	}
	else
	{
		// if we did not enter the section then it should be fine to schedule the
		// continuation here.
		CONTINUE_SECTION_AFTER_PENDING_EVENTS_WITH(continuation);
	}
}

void scheduleContinueInThreadWith( std::function<void()> continuationFunc )
{
    CONTINUE_SECTION_IN_THREAD_WITH(
        [continuationFunc]()
        {
            REQUIRE( !Thread::isCurrentMain() );
            continuationFunc();
        } );    
}

TEST_CASE("CONTINUE_SECTION_IN_THREAD_WITH")
{
    testContinueSectionWith( scheduleContinueInThreadWith );
}

TEST_CASE("CONTINUE_SECTION_IN_THREAD_WITH-expectedFail", "[!shouldfail]")
{
    testContinueSectionWith_expectedFail( scheduleContinueInThreadWith );
}



TEST_CASE("CONTINUE_SECTION_IN_THREAD_WITH-asyncAfterSectionThatHadAsyncContinuation")
{
	bool enteredSection = false;

    SECTION("initialChild")
    {
		enteredSection = true;

        CONTINUE_SECTION_IN_THREAD_WITH( [](){} );
    }

    std::function<void()> continuation =
        []()
        {
            SECTION("asyncChild1")
            {
            }

            SECTION("asyncChild2")
            {
            }
        };


	
	if(enteredSection)
	{
		// we should get a programmingerror here. It is not allowed to schedule a 
		// continuation when one was already scheduled
		REQUIRE_THROWS_PROGRAMMING_ERROR( CONTINUE_SECTION_IN_THREAD_WITH(continuation) );
	}
	else
	{
		// if we did not enter the section then it should be fine to schedule the
		// continuation here.
		CONTINUE_SECTION_IN_THREAD_WITH(continuation);
	}    
}


TEST_CASE("ASYNC_SECTION")
{
    static bool asyncExecuted = false;
    
    bool asyncExecutedBefore = asyncExecuted;

    ASYNC_SECTION("async")
    {
        asyncExecuted = true;
    };

    if(!asyncExecutedBefore)
    {
        // the async section should not have been executed synchronously
        REQUIRE(!asyncExecuted);
    }

    SECTION("afterAsync")
    {
        // the async section have been executed before this next section         
        REQUIRE( asyncExecuted );
    }
}


TEST_CASE("ASYNC_SECTION in ASYNC_SECTION")
{
    static bool asyncExecuted = false;
    static bool innerAsyncExecuted1 = false;
    static bool innerAsyncExecuted2 = false;

    ASYNC_SECTION("async")
    {
        asyncExecuted = true;

        ASYNC_SECTION("innerAsync1")
        {
            innerAsyncExecuted1 = true;
        };

        ASYNC_SECTION("innerAsync2")
        {
            innerAsyncExecuted2 = true;
        };
    };


    SECTION("afterAsync")
    {
        // all async sections should have been executed.
        REQUIRE( asyncExecuted );
        REQUIRE( innerAsyncExecuted1 );
        REQUIRE( innerAsyncExecuted2 );
    }
}

TEST_CASE("ASYNC_SECTION-fail", "[!shouldfail]")
{
    static bool asyncDone = false;
    
    ASYNC_SECTION("async")
    {
        asyncDone = true;
        REQUIRE(false);
    };

    if(asyncDone)
    {
        // when the first section fails in a test case fails then another pass is made
        // afterwards, in which the failed subsection is not entered. That is necessary
        // to ensure that all sub-sections are recognized and executed.
        // So we will get a second pass in which the section is not entered. Make sure that
        // also fails - because we are in a shouldfail test case.
        REQUIRE( false );
    }
}





TEST_CASE( "CONTINUE_SECTION_AFTER_PENDING_EVENTS" )
{
    P<TestData> pData = newObj<TestData>();

    SECTION("notCalledImmediately")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
            pData->callCount++;            
        };

        // should not have been called yet
        REQUIRE( pData->callCount==0 );
    }


    SECTION("notCalledBeforeExitingInitialFunction")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
            pData->callCount++;            
        };

        // even if we wait a while, the continuation should not be called yet
        // (not even if it runs in another thread).
        Thread::sleepMillis(2000);
        REQUIRE( pData->callCount==0 );
    }


    static P<TestData> pCalledBeforeNextSectionData;
    SECTION("calledBeforeNextSection-a")
    {
        pCalledBeforeNextSectionData = pData;

        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
            pData->callCount++;            
        };
    }

    SECTION("calledBeforeNextSection-b")
    {
        REQUIRE( pCalledBeforeNextSectionData!=nullptr );

        // the continuation of the previous section should have been called

        REQUIRE( pCalledBeforeNextSectionData->callCount==1 );
    }

    SECTION("notCalledMultipleTimes")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
            pData->callCount++;            

            REQUIRE( pData->callCount==1 );
        };
    }


    static int subSectionInContinuationMask=0;
    SECTION("subSectionInContinuation-a")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
            subSectionInContinuationMask |= 1;

            SECTION("a")
            {
                SECTION("a1")
                {
                    subSectionInContinuationMask |= 2;
                }

                SECTION("a2")
                {
                    subSectionInContinuationMask |= 4;
                }
            }

            // add another continuation
            SECTION("b")
            {
                CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
                {
                    subSectionInContinuationMask |= 8;

                    SECTION("b1")
                    {
                        subSectionInContinuationMask |= 16;
                    }

                    SECTION("b2")
                    {
                        subSectionInContinuationMask |= 32;
                    }
                };
            }
        };
    }

    SECTION("subSectionInContinuation-b")
    {
        REQUIRE( subSectionInContinuationMask==63 );
    }
}


TEST_CASE("CONTINUE_SECTION_AFTER_PENDING_EVENTS-fail", "[!shouldfail]")
{
    SECTION("exceptionInContinuation")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
            throw std::runtime_error("dummy error");
        };
    }

    SECTION("exceptionAfterContinuationScheduled")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
        };

        throw std::runtime_error("dummy error");
    }

    SECTION("failAfterContinuationScheduled")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
        };
        
        REQUIRE(false);
    }
}

TEST_CASE("CONTINUE_SECTION_AFTER_PENDING_EVENTS-asyncAfterSectionThatHadAsyncContinuation" )
{
	bool enteredSection = false;

    SECTION("initialChild")
    {
		enteredSection = true;
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
        };
    }

	if(enteredSection)
	{
		// we should get a programmingerror here. It is not allowed to schedule a 
		// continuation when one was already scheduled
		REQUIRE_THROWS_PROGRAMMING_ERROR(
            CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
            {
            }; );
	}
	else
	{
		// if we did not enter the section then it should be fine to schedule the
		// continuation here.
        CONTINUE_SECTION_AFTER_PENDING_EVENTS(=)
        {
        };
	}
}



static bool continueAfterPendingEventsComplicated_Started = false;
static bool continueAfterPendingEventsComplicated_Sub2Called = false;

TEST_CASE("CONTINUE_SECTION_AFTER_PENDING_EVENTS-complicated" )
{
    static bool async1Called=false;
    static bool async2Called=false;

    continueAfterPendingEventsComplicated_Started = true;

    SECTION("a")
    {
        CONTINUE_SECTION_AFTER_PENDING_EVENTS()
        {
            SECTION("sub")
            {
                CONTINUE_SECTION_AFTER_PENDING_EVENTS()
                {
                    async1Called = true;

                    CONTINUE_SECTION_AFTER_PENDING_EVENTS()
                    {
                        async2Called = true;
                    };
                };
            }

            // we also want to verify that sub2 is actually executed.
            // This is quite difficult, since there is no code of the test case
            // that is guaranteed to be called afterwards. So we cannot
            // do a test at the end to see which sections were called.
            // So we do the best we can: add another test case after it that tests
            // IF this test case was executed before that sub2 was executed.
            // Since we cannot control which test cases are executed, this second test does
            // not always have an effect, but at least it will work if all tests are executed
            // (either in alphabetical or code order).
    
            SECTION("sub2")
            {
                continueAfterPendingEventsComplicated_Sub2Called = true;

                REQUIRE( async1Called );
                REQUIRE( async2Called );        
            }
        };
    };
}

TEST_CASE("CONTINUE_SECTION_AFTER_PENDING_EVENTS-complicated-B" )
{
    // see comment in previous test case for explanation

    if(continueAfterPendingEventsComplicated_Started)
    {
        REQUIRE( continueAfterPendingEventsComplicated_Sub2Called );
    }
}



TEST_CASE( "CONTINUE_SECTION_IN_THREAD" )
{
    P<TestData> pData = newObj<TestData>();

    SECTION("notCalledImmediately")
    {
        CONTINUE_SECTION_IN_THREAD(=)
        {
            pData->callCount++;            
        };

        // should not have been called yet
        REQUIRE( pData->callCount==0 );
    }


    SECTION("notCalledBeforeExitingInitialFunction")
    {
        CONTINUE_SECTION_IN_THREAD(=)
        {
            pData->callCount++;            
        };

        // even if we wait a while, the continuation should not be called yet
        // (not even if it runs in another thread).
        Thread::sleepMillis(2000);
        REQUIRE( pData->callCount==0 );
    }


    static P<TestData> pCalledBeforeNextSectionData;
    SECTION("calledBeforeNextSection-a")
    {
        pCalledBeforeNextSectionData = pData;

        CONTINUE_SECTION_IN_THREAD(=)
        {
            pData->callCount++;            
        };
    }

    SECTION("calledBeforeNextSection-b")
    {
        REQUIRE( pCalledBeforeNextSectionData!=nullptr );

        // the continuation of the previous section should have been called

        REQUIRE( pCalledBeforeNextSectionData->callCount==1 );
    }

    SECTION("notCalledMultipleTimes")
    {
        CONTINUE_SECTION_IN_THREAD(=)
        {
            pData->callCount++;            

            REQUIRE( pData->callCount==1 );
        };
    }


    static int subSectionInContinuationMask=0;
    SECTION("subSectionInContinuation-a")
    {
        CONTINUE_SECTION_IN_THREAD(=)
        {
            subSectionInContinuationMask |= 1;

            SECTION("a")
            {
                SECTION("a1")
                {
                    subSectionInContinuationMask |= 2;
                }

                SECTION("a2")
                {
                    subSectionInContinuationMask |= 4;
                }
            }

            // add another continuation
            SECTION("b")
            {
                CONTINUE_SECTION_IN_THREAD(=)
                {
                    subSectionInContinuationMask |= 8;

                    SECTION("b1")
                    {
                        subSectionInContinuationMask |= 16;
                    }

                    SECTION("b2")
                    {
                        subSectionInContinuationMask |= 32;
                    }
                };
            }
        };
    }

    SECTION("subSectionInContinuation-b")
    {
        REQUIRE( subSectionInContinuationMask==63 );
    }
}


TEST_CASE("CONTINUE_SECTION_IN_THREAD-fail", "[!shouldfail]")
{
    SECTION("exceptionInContinuation")
    {
        CONTINUE_SECTION_IN_THREAD(=)
        {
            throw std::runtime_error("dummy error");
        };
    }

    SECTION("exceptionAfterContinuationScheduled")
    {
        CONTINUE_SECTION_IN_THREAD(=)
        {
        };

        throw std::runtime_error("dummy error");
    }

    SECTION("failAfterContinuationScheduled")
    {
        CONTINUE_SECTION_IN_THREAD(=)
        {
        };
        
        REQUIRE(false);
    }
}

TEST_CASE("CONTINUE_SECTION_IN_THREAD-asyncAfterSectionThatHadAsyncContinuation" )
{
	bool enteredSection = false;

    SECTION("initialChild")
    {
		enteredSection = true;
        CONTINUE_SECTION_IN_THREAD(=)
        {
        };
    }

	if(enteredSection)
	{
		// we should get a programmingerror here. It is not allowed to schedule a 
		// continuation when one was already scheduled
		REQUIRE_THROWS_PROGRAMMING_ERROR(
            CONTINUE_SECTION_IN_THREAD(=)
            {
            }; );
	}
	else
	{
		// if we did not enter the section then it should be fine to schedule the
		// continuation here.
        CONTINUE_SECTION_IN_THREAD(=)
        {
        };
	}
}
