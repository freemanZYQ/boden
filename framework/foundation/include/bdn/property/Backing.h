#pragma once

#include <bdn/property/Property.h>
#include <bdn/property/IValueAccessor.h>
#include <bdn/SimpleNotifier.h>

#include <memory>

namespace bdn
{

    template <class ValType> class Backing : virtual public IValueAccessor<ValType>
    {
      public:
        class Proxy
        {
          public:
            Proxy(ValType value) : _value(value) {}

            const ValType *operator->() const { return &_value; }

          private:
            ValType _value;
        };

        using value_accessor_t = IValueAccessor<ValType>;
        using value_accessor_t_ptr = std::shared_ptr<value_accessor_t>;
        using notifier_t = SimpleNotifier<std::shared_ptr<value_accessor_t>>;
        using notifier_t_ptr = std::shared_ptr<notifier_t>;
        using property_t = Property<ValType>;
        using property_t_ptr = std::shared_ptr<property_t>;

      public:
        Backing() : _pOnChange(std::make_shared<notifier_t>()) {}

        std::shared_ptr<Backing<ValType>> shared_from_this()
        {
            return std::dynamic_pointer_cast<Backing<ValType>>(IValueAccessor<ValType>::shared_from_this());
        }

        virtual ValType get() const = 0;
        virtual void set(const ValType &value, bool notify = true) = 0;

        virtual Proxy proxy() const { return Proxy(get()); }

        virtual notifier_t &onChange() const { return *(_pOnChange.get()); }

        virtual void bind(std::shared_ptr<Backing<ValType>> sourceBacking)
        {
            sourceBacking->onChange() += weakMethod(shared_from_this(), &Backing::bindSourceChanged);
            bindSourceChanged(sourceBacking);
        }

      public:
        void bindSourceChanged(value_accessor_t_ptr pValue) { set(pValue->get()); }

      protected:
        mutable notifier_t_ptr _pOnChange;
    };
}