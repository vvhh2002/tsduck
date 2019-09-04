// Sample TSDuck extension.
// Definition of the foo_descriptor
//
// Layout of a foo_descriptor:
//
//    descriptor_tag           8 bits = 0xE8
//    descriptor_length        8 bits
//    for(i=0;i<N;i++) {
//        name_char            8 bits
//    }

#pragma once
#include "tsduck.h"
#include "foo.h"

namespace foo {

    class FOODLL FooDescriptor : public ts::AbstractDescriptor
    {
    public:
        // FooDescriptor public members:
        ts::UString name; // Foo name.

        // Default constructor.
        FooDescriptor(const ts::UString& name = ts::UString());

        // Constructor from a binary descriptor
        FooDescriptor(ts::DuckContext& duck, const ts::Descriptor& bin);

        // Inherited methods
        virtual void serialize(ts::DuckContext&, ts::Descriptor&) const override;
        virtual void deserialize(ts::DuckContext&, const ts::Descriptor&) override;
        virtual void buildXML(ts::DuckContext&, ts::xml::Element*) const override;
        virtual void fromXML(ts::DuckContext&, const ts::xml::Element*) override;
        DeclareDisplayDescriptor();
    };

}
