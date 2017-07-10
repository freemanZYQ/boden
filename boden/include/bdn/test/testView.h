#ifndef BDN_TEST_testView_H_
#define BDN_TEST_testView_H_


#include <bdn/test/MockUiProvider.h>
#include <bdn/test/ViewWithTestExtensions.h>
#include <bdn/test/testCalcPreferredSize.h>
#include <bdn/test/MockViewCore.h>

namespace bdn
{
namespace test
{

    
template<class ViewType>
class ViewTestPreparer : public Base
{
public:
    ViewTestPreparer()
    {
        _pUiProvider = newObj<MockUiProvider>();
        _pWindow = newObj<Window>(_pUiProvider);
    }

    P<MockUiProvider> getUiProvider()
    {
        return _pUiProvider;
    }

    P<Window> getWindow()
    {
        return _pWindow;
    }

    P< ViewWithTestExtensions<ViewType> > createView()
    {
        P< ViewWithTestExtensions<ViewType> > pView = newObj< ViewWithTestExtensions<ViewType> >();

        _pWindow->setContentView(pView);

        return pView;
    }

    void createLocalView()
    {
        ViewType view;
    }
    
protected:
    P<MockUiProvider>       _pUiProvider;
    P<Window>               _pWindow;
};



template<>
class ViewTestPreparer<Window> : public Base
{
public:
    ViewTestPreparer()
    {
        _pUiProvider = newObj<MockUiProvider>();
    }

    P<MockUiProvider> getUiProvider()
    {
        return _pUiProvider;
    }

    P<Window> getWindow()
    {
        return _pWindow;
    }

    P< ViewWithTestExtensions<Window> > createView()
    {
        P<ViewWithTestExtensions<Window> > pWindow = newObj< ViewWithTestExtensions<Window> >( _pUiProvider );

        _pWindow = pWindow;

        return pWindow;
    }

    void createLocalView()
    {
        Window window(_pUiProvider);
    }

protected:
    P<MockUiProvider>       _pUiProvider;
    P<Window>               _pWindow;
};


template<class ViewType>
inline bool shouldViewBeInitiallyVisible()
{
    return true;
}

template<>
inline bool shouldViewBeInitiallyVisible<Window>()
{
    return false;
}



template<class ViewType>
inline bool shouldViewHaveParent()
{
    return true;
}

template<>
inline bool shouldViewHaveParent<Window>()
{
    return false;
}








/** Helper function that performs an operation on a view object and verifies the result afterwards.

    The operation is performed twice: once from the main thread and once from another thread (if the target
    platform supports multithreading).

    @param pView the view to perform the operation on
    @param opFunc a function object that performs the action on the view
    @param verifyFunc a function object that verifies that the view is in the expected state after the action.
    @param expectedLayoutUpdates the number of layout updates the operation should trigger. This is usually
        either 0 (layout should not be updated) or 1 (layout should be updated).
*/
template<class ViewType>
inline void testViewOp(P< ViewWithTestExtensions<ViewType> > pView,
                std::function<void()> opFunc,
                std::function<void()> verifyFunc,
                int expectedLayoutUpdates )
{
    // schedule the test asynchronously, so that the initial sizing
	// info update from the view construction is already done.

	ASYNC_SECTION("mainThread", pView, opFunc, verifyFunc, expectedLayoutUpdates)
	{
        int initialLayoutCount = cast<MockViewCore>(pView->getViewCore())->getLayoutCount();

		opFunc();

        // layout updates should never happen immediately. We want them
		// to happen asynchronously, so that multiple changes can be handled
		// together with a single update.
		BDN_REQUIRE( cast<MockViewCore>(pView->getViewCore())->getLayoutCount() == initialLayoutCount );	

        // the results of the change may depend on notification calls. Those are posted
        // to the main event queue. So we need to process those now before we verify.
        BDN_CONTINUE_SECTION_WHEN_IDLE(pView, initialLayoutCount, expectedLayoutUpdates, verifyFunc)
        {
		    verifyFunc();

		    // layout also happen asynchronously. Process all events that were added in the initial
            // notification round and then check them.
		    
            BDN_CONTINUE_SECTION_WHEN_IDLE(pView, initialLayoutCount, expectedLayoutUpdates, verifyFunc)
            {
                BDN_REQUIRE( cast<MockViewCore>(pView->getViewCore())->getLayoutCount() == initialLayoutCount + expectedLayoutUpdates );
            };
        };
	};

#if BDN_HAVE_THREADS

    // schedule the test asynchronously, so that the layout update from the view construction is already done.
	ASYNC_SECTION("otherThread", pView, opFunc, verifyFunc, expectedLayoutUpdates)
	{
        int initialLayoutCount = cast<MockViewCore>(pView->getViewCore())->getLayoutCount();

		// note that we call get on the future object, so that we wait until the
		// other thread has finished (so that any changes have been scheduled)
		Thread::exec( opFunc ).get();		

		// any changes made to properties by the opFunc are asynchronously scheduled.
		// So they have not actually been made in the core yet,

		// we want to wait until the changes have actually been made in the core.
		// So do another async call. That one will be executed after the property
		// changes.

        BDN_CONTINUE_SECTION_WHEN_IDLE( pView, verifyFunc, initialLayoutCount, expectedLayoutUpdates )
		{
            // the results of the change may depend on notification calls. Those are posted
            // to the main event queue. So we need to process those now before we verify.
            BDN_CONTINUE_SECTION_WHEN_IDLE(pView, initialLayoutCount, expectedLayoutUpdates, verifyFunc)
            {
		        verifyFunc();

		        // layout updates also happen asynchronously. Process all events that were added in the initial
                // notification round and then check them.
		    
                BDN_CONTINUE_SECTION_WHEN_IDLE(pView, initialLayoutCount, expectedLayoutUpdates, verifyFunc)
                {
                    BDN_REQUIRE( cast<MockViewCore>(pView->getViewCore())->getLayoutCount() == initialLayoutCount + expectedLayoutUpdates );
                };
            };
        };
	};

#endif

}


template<class ViewType >
inline void testView_Continue(
    P< ViewTestPreparer<ViewType> >         pPreparer,
    int                                     initialCoresCreated,
    P<Window>                               pWindow,
    P< ViewWithTestExtensions<ViewType> >   pView,
    P<bdn::test::MockViewCore>              pCore);

template<class ViewType >
inline void testView()
{
    P< ViewTestPreparer<ViewType> > pPreparer = newObj< ViewTestPreparer<ViewType> >();

    int initialCoresCreated = pPreparer->getUiProvider()->getCoresCreated();

    SECTION("onlyNewAllocAllowed")
	{
		BDN_REQUIRE_THROWS_PROGRAMMING_ERROR( pPreparer->createLocalView() );

		BDN_REQUIRE( pPreparer->getUiProvider()->getCoresCreated()==initialCoresCreated );
	}

	P< ViewWithTestExtensions<ViewType> > pView = pPreparer->createView();
	BDN_REQUIRE( pPreparer->getUiProvider()->getCoresCreated()==initialCoresCreated+1 );

	P<bdn::test::MockViewCore> pCore = cast<bdn::test::MockViewCore>( pView->getViewCore() );
	BDN_REQUIRE( pCore!=nullptr );

    P<Window> pWindow = pPreparer->getWindow();

    
    // Normally the default for a view's visible property is true.
    // But for top level Windows, for example, the default is false. This is a change that is done in the constructor
    // of the Window object. At that point there are no subscribers for the property's change event, BUT a notification
    // is still scheduled. And if there are subscribers at the point when the notification
    // is actually handled then we will get a visibility change recorded. So the expected
    // value here is 1.
    // In general that means that we expect a visible change count of 0 for views that
    // are initially visible and a change count of 1 for those that are initially invisible.
    int initialVisibleChangeCount = shouldViewBeInitiallyVisible<ViewType>() ? 0 : 1;
    
    BDN_CONTINUE_SECTION_WHEN_IDLE( pPreparer, initialCoresCreated, pWindow, pView, pCore, initialVisibleChangeCount )
    {        
	    SECTION("initialViewState")
	    {
            // the core should initialize its properties from the outer window when it is created.
		    // The outer window should not set them manually after construction.		
            BDN_REQUIRE( pCore->getPaddingChangeCount()==0 );
		    BDN_REQUIRE( pCore->getParentViewChangeCount()==0 );

            BDN_REQUIRE( pCore->getVisibleChangeCount()==initialVisibleChangeCount );
            
            BDN_REQUIRE( pView->visible() == shouldViewBeInitiallyVisible<ViewType>() );
		    
            BDN_REQUIRE( pView->margin() == UiMargin( UiLength() ) );
		    BDN_REQUIRE( pView->padding().get().isNull() );

		    BDN_REQUIRE( pView->horizontalAlignment() == View::HorizontalAlignment::left );
		    BDN_REQUIRE( pView->verticalAlignment() == View::VerticalAlignment::top );

            BDN_REQUIRE( pView->preferredSizeHint() == Size::none() );
            BDN_REQUIRE( pView->preferredSizeMinimum() == Size::none() );
            BDN_REQUIRE( pView->preferredSizeMaximum() == Size::none() );

		    BDN_REQUIRE( pView->getUiProvider().getPtr() == pPreparer->getUiProvider() );

            if( shouldViewHaveParent<ViewType>() )
		        BDN_REQUIRE( pView->getParentView()==cast<View>(pPreparer->getWindow()) );
            else
                BDN_REQUIRE( pView->getParentView()==nullptr );

		    BDN_REQUIRE( pView->getViewCore().getPtr() == pCore );


            // the view should not have any child views
		    std::list< P<View> > childViews;
		    pView->getChildViews(childViews);
		    BDN_REQUIRE( childViews.empty() );	

	    }

        SECTION("multiple invalidateSizingInfo calls cause single layout")
        {
            int layoutCountBefore = pCore->getLayoutCount();

            pView->invalidateSizingInfo( View::InvalidateReason::customChange );
            pView->invalidateSizingInfo( View::InvalidateReason::customChange  );

            CONTINUE_SECTION_WHEN_IDLE(pPreparer, pView, pCore, layoutCountBefore)
            {
                REQUIRE( pCore->getLayoutCount() == layoutCountBefore+1 );
            };
        }
    
        SECTION("parentViewNullAfterParentDestroyed")
	    {
            P<ViewType> pView;

            {
                ViewTestPreparer<ViewType> preparer2;

                pView = preparer2.createView();

                if(shouldViewHaveParent<ViewType>())
                    BDN_REQUIRE( pView->getParentView() != nullptr);
                else
                    BDN_REQUIRE( pView->getParentView() == nullptr);
            }

            // preparer2 is now gone, so the parent view is not referenced there anymore.
            // But there may still be a scheduled sizing info update pending that holds a
            // reference to the window or child view.
            // Since we want the window to be destroyed, we do the remaining test asynchronously
            // after all pending operations are done.

            CONTINUE_SECTION_WHEN_IDLE(pView, pPreparer)
            {                
                BDN_REQUIRE( pView->getParentView() == nullptr);	    
            };
	    }


	    SECTION("changeViewProperty")
	    {
		    SECTION("visible")
		    {
			    testViewOp<ViewType>(
				    pView,
				    [pView, pWindow]()
				    {
					    pView->visible() = !shouldViewBeInitiallyVisible<ViewType>();
				    },
				    [pCore, pView, pWindow, initialVisibleChangeCount]()
				    {
					    BDN_REQUIRE( pCore->getVisibleChangeCount()==initialVisibleChangeCount+1 );
					    BDN_REQUIRE( pCore->getVisible() == !shouldViewBeInitiallyVisible<ViewType>() );	
				    },
				    0	// should NOT have caused a sizing info update
				    );
		    }
	
		    SECTION("margin")
		    {
			    UiMargin m( UiLength::sem(1),
                            UiLength::sem(2),
                            UiLength::sem(3),
                            UiLength::sem(4) );

			    testViewOp<ViewType>( 
				    pView,
				    [pView, m, pWindow]()
				    {
					    pView->margin() = m;
				    },
				    [pCore, m, pView, pWindow]()
				    {
                        // margin should still have the value we set
                        REQUIRE( pView->margin().get() == m );
				    },
				    0	// should NOT have caused a sizing info update
				    );
		    }

		    SECTION("padding")
		    {
			    UiMargin m(UiLength::sem(1), UiLength::sem(2), UiLength::sem(3), UiLength::sem(4) );

			    testViewOp<ViewType>( 
				    pView,
				    [pView, m, pWindow]()
				    {
					    pView->padding() = m;
				    },
				    [pCore, m, pView, pWindow]()
				    {
					    BDN_REQUIRE( pCore->getPaddingChangeCount()==1 );
					    BDN_REQUIRE( pCore->getPadding() == m);
				    },
				    1	// should have caused a sizing info update
				    );
		    }

		    SECTION("adjustAndSetBounds")
		    {
                SECTION("no need to adjust")
                {
			        Rect bounds(1, 2, 3, 4);

                    int boundsChangeCountBefore = pCore->getBoundsChangeCount();

			        testViewOp<ViewType>( 
				        pView,
				        [pView, bounds, pWindow]()
				        {
					        Rect adjustedBounds = pView->adjustAndSetBounds(bounds);
                            REQUIRE( adjustedBounds==bounds );
				        },
				        [pCore, bounds, pView, pWindow, boundsChangeCountBefore]()
				        {
					        BDN_REQUIRE( pCore->getBoundsChangeCount()==boundsChangeCountBefore+1 );
					        BDN_REQUIRE( pCore->getBounds() == bounds );

                            // the view's position and size properties should reflect the new bounds
					        BDN_REQUIRE( pView->position() == bounds.getPosition() );
                            BDN_REQUIRE( pView->size() == bounds.getSize() );
				        },
				        0	// should NOT have caused a sizing info update
				        );
                }

                SECTION("need adjustment")
                {
                    Rect bounds(1.3, 2.4, 3.1, 4.9);

                    // the mock view uses 3 pixels per DIP. Coordinates should be rounded to the
                    // NEAREST value
                    Rect expectedAdjustedBounds( 1+1.0/3, 2 + 1.0/3, 3, 5 );

                    int boundsChangeCountBefore = pCore->getBoundsChangeCount();

			        testViewOp<ViewType>( 
				        pView,
				        [pView, bounds, expectedAdjustedBounds, pWindow]()
				        {
					        Rect adjustedBounds = pView->adjustAndSetBounds(bounds);                            
                            REQUIRE( adjustedBounds==expectedAdjustedBounds );
				        },
				        [pCore, bounds, expectedAdjustedBounds, pView, pWindow, boundsChangeCountBefore]()
				        {
					        BDN_REQUIRE( pCore->getBoundsChangeCount()==boundsChangeCountBefore+1 );
                            BDN_REQUIRE( pCore->getBounds()==expectedAdjustedBounds );

                            // the view's position and size properties should reflect the new, adjusted bounds
					        BDN_REQUIRE( pView->position() == expectedAdjustedBounds.getPosition() );
                            BDN_REQUIRE( pView->size() == expectedAdjustedBounds.getSize() );
				        },
				        0	// should NOT have caused a sizing info update
				        );

                }
		    }


            SECTION("adjustBounds")
		    {
                SECTION("no need to adjust")
                {
			        Rect bounds(1, 2, 3, 4);
                    Rect origBounds = pCore->getBounds();

                    std::list<RoundType> roundTypes{RoundType::nearest, RoundType::up, RoundType::down};

                    for(RoundType positionRoundType: roundTypes)
                    {
                        for(RoundType sizeRoundType: roundTypes)
                        {
                            SECTION( "positionRoundType: "+std::to_string((int)positionRoundType)+", "+std::to_string((int)sizeRoundType) )
                            {
                                Rect adjustedBounds = pView->adjustBounds(bounds, positionRoundType, sizeRoundType);

                                // no adjustments are necessary. So we should always get out the same that we put in
                                REQUIRE( adjustedBounds==bounds );

                                // view properties should not have changed
                                REQUIRE( pView->position() == origBounds.getPosition() );
                                REQUIRE( pView->size() == origBounds.getSize() );

                                // the core bounds should not have been updated
                                REQUIRE( pCore->getBounds() == origBounds );
                            }
                        }
                    }
                }

                SECTION("need adjustments")
                {
			        Rect bounds(1.3, 2.4, 3.1, 4.9);
                    Rect origBounds = pCore->getBounds();

                    std::list<RoundType> roundTypes{RoundType::nearest, RoundType::up, RoundType::down};

                    for(RoundType positionRoundType: roundTypes)
                    {
                        for(RoundType sizeRoundType: roundTypes)
                        {
                            SECTION( "positionRoundType: "+std::to_string((int)positionRoundType)+", "+std::to_string((int)sizeRoundType) )
                            {
                                Rect adjustedBounds = pView->adjustBounds(bounds, positionRoundType, sizeRoundType);

                                Point expectedPos;
                                if(positionRoundType==RoundType::down)
                                    expectedPos = Point(1, 2 + 1.0/3);
                                else if(positionRoundType==RoundType::up)
                                    expectedPos = Point(1 + 1.0/3, 2 + 2.0/3);
                                else
                                    expectedPos = Point(1 + 1.0/3, 2 + 1.0/3);

                                Size expectedSize;
                                if(sizeRoundType==RoundType::down)
                                    expectedSize = Size(3, 4+2.0/3);
                                else if(sizeRoundType==RoundType::up)
                                    expectedSize = Size(3+1.0/3, 5);
                                else
                                    expectedSize = Size(3, 5);

                                REQUIRE( adjustedBounds==Rect(expectedPos, expectedSize) );

                                // view properties should not have changed
                                REQUIRE( pView->position() == origBounds.getPosition() );
                                REQUIRE( pView->size() == origBounds.getSize() );

                                // the core bounds should not have been updated
                                REQUIRE( pCore->getBounds() == origBounds );
                            }
                        }
                    }
                }
            }
	    }

        SECTION("preferredSize")
            bdn::test::_testCalcPreferredSize<ViewType, View>(pView, pView, pPreparer);

	    SECTION("multiplePropertyChangesThatInfluenceSizing")
	    {
		    testViewOp<ViewType>(
			    pView,

			    [pView, pWindow]()
			    {
				    pView->padding() = UiMargin(UiLength::sem(7), UiLength::sem(8), UiLength::sem(9), UiLength::sem(10));
				    pView->padding() = UiMargin(UiLength::sem(6), UiLength::sem(7), UiLength::sem(8), UiLength::sem(9) );
			    },

			    [pCore, pView, pWindow]()
			    {
				    // padding changed twice
				    BDN_REQUIRE( pCore->getPaddingChangeCount()==2 );
				    BDN_REQUIRE( pCore->getPadding() == UiMargin(UiLength::sem(6), UiLength::sem(7), UiLength::sem(8), UiLength::sem(9) ));
			    },

			    1	// should cause a single(!) sizing info update

			    );		
	    }


#if BDN_HAVE_THREADS
        SECTION("core deinit called from main thread")
        {
            struct Data : public Base
            {
                P<ViewType> pView;
                P< ViewTestPreparer<ViewType> > pPreparer2;
            };

            P<Data> pData = newObj<Data>();

            pData->pPreparer2 = newObj< ViewTestPreparer<ViewType> >();

            pData->pView = pData->pPreparer2->createView();

            // the view should have a core
            REQUIRE( pData->pView->getViewCore()!=nullptr );

            CONTINUE_SECTION_IN_THREAD( pData )
            {
                // release the view and the preparer here.
                // That will cause the corresponding core to be deleted.
                // And the mock core object will verify that that happened
                // in the main thread.
                pData->pPreparer2 = nullptr;
                pData->pView = nullptr;
            };
        }

        SECTION("core initialized from main thread")
        {
            CONTINUE_SECTION_IN_THREAD()
            {
                P< ViewTestPreparer<ViewType> > pPreparer2 = newObj< ViewTestPreparer<ViewType> >();

                // create the view. The core creation should be moved to the main thread
                // automatically. The mock core constructor will verify this, so we will get
                // a failed REQUIRE here if the view calls the constructor from the main thread.
                P<ViewType> pView = pPreparer2->createView();
            };

        }
#endif
    };
}

}
}

#endif

