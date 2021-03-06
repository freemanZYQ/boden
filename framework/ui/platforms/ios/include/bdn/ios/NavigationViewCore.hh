#pragma once

#import <bdn/ios/ViewCore.hh>
#include <bdn/ui/NavigationView.h>

@interface BodenUINavigationControllerContainerView : UIView <UIViewWithFrameNotification>
@property(nonatomic, assign) std::weak_ptr<bdn::ui::ios::ViewCore> viewCore;
@property UINavigationController *navController;
@end

namespace bdn::ui::ios
{
    class NavigationViewCore : public ViewCore, public NavigationView::Core
    {
      public:
        NavigationViewCore(const std::shared_ptr<ViewCoreFactory> &viewCoreFactory);

      public:
        void init() override;

        void frameChanged() override;
        void onGeometryChanged(Rect newGeometry) override;

        void pushView(std::shared_ptr<View> view, String title) override;
        void popView() override;

        std::list<std::shared_ptr<View>> childViews() override;

      private:
        UINavigationController *getNavigationController();

        std::shared_ptr<ContainerView> getCurrentContainer();
        std::shared_ptr<View> getCurrentUserView();
    };
}
