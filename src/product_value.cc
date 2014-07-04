#include <loom/product_value.hpp>

namespace loom
{

void ValueSplitter::init (
        const ValueSchema & schema,
        const std::vector<uint32_t> & full_to_part,
        size_t part_count)
{
    LOOM_ASSERT_EQ(schema.total_size(), full_to_part.size());
    this->schema = schema;
    this->full_to_part = full_to_part;

    parts.clear();
    parts.resize(part_count);

    detail::BlockIterator block;
    size_t full_pos = 0;
    for (block(schema.booleans_size); block.ok(full_pos); ++full_pos) {
        auto & part = parts[full_to_part[full_pos]];
        part.part_to_full.push_back(full_pos);
        part.schema.booleans_size += 1;
    }
    for (block(schema.counts_size); block.ok(full_pos); ++full_pos) {
        auto & part = parts[full_to_part[full_pos]];
        part.part_to_full.push_back(full_pos);
        part.schema.counts_size += 1;
    }
    for (block(schema.reals_size); block.ok(full_pos); ++full_pos) {
        auto & part = parts[full_to_part[full_pos]];
        part.part_to_full.push_back(full_pos);
        part.schema.reals_size += 1;
    }
    LOOM_ASSERT_EQ(full_pos, full_to_part.size());
}

struct ValueSplitter::split_value_all_fun
{
    const ValueSplitter & splitter;
    const ProductValue & full_value;
    std::vector<ProductValue> & partial_values;
    detail::BlockIterator block;
    size_t full_pos;

    template<class FieldType>
    void operator() (FieldType *, size_t size)
    {
        typedef protobuf::Fields<FieldType> Fields;
        const auto & full_fields = Fields::get(full_value);
        for (block(size); block.ok(full_pos); ++full_pos) {
            auto partid = splitter.full_to_part[full_pos];
            auto & partial_value = partial_values[partid];
            Fields::get(partial_value).Add(full_fields.Get(full_pos));
        }
    }
};

struct ValueSplitter::split_value_dense_fun
{
    const ValueSplitter & splitter;
    const ProductValue & full_value;
    std::vector<ProductValue> & partial_values;
    detail::BlockIterator block;
    size_t full_pos;

    template<class FieldType>
    void operator() (FieldType *, size_t size)
    {
        typedef protobuf::Fields<FieldType> Fields;
        const auto & full_fields = Fields::get(full_value);
        for (block(size); block.ok(full_pos); ++full_pos) {
            auto partid = splitter.full_to_part[full_pos];
            auto & partial_value = partial_values[partid];
            bool observed = full_value.observed().dense(full_pos);
            partial_value.mutable_observed()->add_dense(observed);
            if (observed) {
                Fields::get(partial_value).Add(
                    full_fields.Get(block.get(full_pos)));
            }
        }
    }
};

struct ValueSplitter::split_value_sparse_fun
{
    const ValueSplitter & splitter;
    const ProductValue & full_value;
    std::vector<ProductValue> & partial_values;
    detail::BlockIterator block;
    decltype(full_value.observed().sparse().begin()) i;
    decltype(full_value.observed().sparse().begin()) end;

    template<class FieldType>
    void operator() (FieldType *, size_t size)
    {
        typedef protobuf::Fields<FieldType> Fields;
        const auto & full_fields = Fields::get(full_value);
        for (block(size); i != end and block.ok(*i); ++i) {
            auto full_pos = *i;
            auto partid = splitter.full_to_part[full_pos];
            auto part_pos = splitter.parts[partid].part_to_full[full_pos];
            auto & partial_value = partial_values[partid];
            partial_value.mutable_observed()->add_sparse(part_pos);
            Fields::get(partial_value).Add(
                full_fields.Get(block.get(full_pos)));
        }
    }
};

void ValueSplitter::split (
        const ProductValue & full_value,
        std::vector<ProductValue> & partial_values) const
{
    validate(full_value);

    partial_values.resize(parts.size());
    auto sparsity = full_value.observed().sparsity();
    for (auto & partial_value : partial_values) {
        partial_value.Clear();
        partial_value.mutable_observed()->set_sparsity(sparsity);
    }

    switch (sparsity) {
        case ProductValue::Observed::ALL: {
            split_value_all_fun fun = {
                *this,
                full_value,
                partial_values,
                detail::BlockIterator(),
                0};
            schema.for_each_datatype(fun);
            LOOM_ASSERT1(
                fun.full_pos == full_to_part.size(),
                "programmer error");
        } break;

        case ProductValue::Observed::DENSE: {
            split_value_dense_fun fun = {
                *this,
                full_value,
                partial_values,
                detail::BlockIterator(),
                0};
            schema.for_each_datatype(fun);
            LOOM_ASSERT1(
                fun.full_pos == full_to_part.size(),
                "programmer error");
        } break;

        case ProductValue::Observed::SPARSE: {
            split_value_sparse_fun fun = {
                *this,
                full_value,
                partial_values,
                detail::BlockIterator(),
                full_value.observed().sparse().begin(),
                full_value.observed().sparse().end()};
            schema.for_each_datatype(fun);
            LOOM_ASSERT1(fun.i == fun.end, "programmer error");
        } break;
    }

    validate(partial_values);
}

struct ValueSplitter::split_observed_dense_fun
{
    const ValueSplitter & splitter;
    const ProductValue & full_value;
    std::vector<ProductValue> & partial_values;
    detail::BlockIterator block;
    size_t full_pos;

    template<class FieldType>
    void operator() (FieldType *, size_t size)
    {
        for (block(size); block.ok(full_pos); ++full_pos) {
            auto partid = splitter.full_to_part[full_pos];
            auto & partial_value = partial_values[partid];
            bool observed = full_value.observed().dense(full_pos);
            partial_value.mutable_observed()->add_dense(observed);
        }
    }
};

void ValueSplitter::split_observed (
        const ProductValue & full_value,
        std::vector<ProductValue> & partial_values) const
{
    validate(full_value);
    auto sparsity = full_value.observed().sparsity();
    LOOM_ASSERT_EQ(sparsity, ProductValue::Observed::DENSE);

    partial_values.resize(parts.size());
    for (auto & partial_value : partial_values) {
        partial_value.Clear();
        partial_value.mutable_observed()->set_sparsity(sparsity);
    }

    split_observed_dense_fun fun = {
        *this,
        full_value,
        partial_values,
        detail::BlockIterator(),
        0};
    schema.for_each_datatype(fun);
    LOOM_ASSERT1(fun.full_pos == full_to_part.size(), "programmer error");
}

struct ValueSplitter::join_value_dense_fun
{
    const ValueSplitter & splitter;
    ProductValue & full_value;
    const std::vector<ProductValue> & partial_values;
    size_t full_pos;

    template<class FieldType>
    void operator() (FieldType *, size_t size)
    {
        if (size) {
            auto & absolute_pos_list = splitter.absolute_pos_list_;
            auto & packed_pos_list = splitter.packed_pos_list_;
            typedef protobuf::Fields<FieldType> Fields;
            auto & full_fields = Fields::get(full_value);
            std::fill(packed_pos_list.begin(), packed_pos_list.end(), 0);
            for (size_t end = full_pos + size; full_pos < end; ++full_pos) {
                auto partid = splitter.full_to_part[full_pos];
                auto & partial_value = partial_values[partid];
                auto & absolute_pos = absolute_pos_list[partid];
                bool observed = partial_value.observed().dense(absolute_pos++);
                full_value.mutable_observed()->add_dense(observed);
                if (observed) {
                    auto & packed_pos = packed_pos_list[partid];
                    auto & partial_fields = Fields::get(partial_value);
                    full_fields.Add(partial_fields.Get(packed_pos++));
                }
            }
        }
    }
};

void ValueSplitter::join (
        ProductValue & full_value,
        const std::vector<ProductValue> & partial_values) const
{
    //LOOM_DEBUG(partial_values);
    validate(partial_values);
    auto sparsity = partial_values[0].observed().sparsity();
    LOOM_ASSERT_EQ(sparsity, ProductValue::Observed::DENSE);

    full_value.Clear();
    full_value.mutable_observed()->set_sparsity(sparsity);
    absolute_pos_list_.clear();
    absolute_pos_list_.resize(parts.size(), 0);
    packed_pos_list_.resize(parts.size());
    join_value_dense_fun fun = {*this, full_value, partial_values, 0};
    schema.for_each_datatype(fun);

    if (LOOM_DEBUG_LEVEL >= 1) {
        LOOM_ASSERT_EQ(fun.full_pos, full_to_part.size());
    }

    validate(full_value);
}

} // namespace loom
