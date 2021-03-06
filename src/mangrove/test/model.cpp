// Copyright 2016 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "catch.hpp"

#include <bsoncxx/builder/stream/document.hpp>

#include <boson/stdx/optional.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <mangrove/macros.hpp>
#include <mangrove/model.hpp>
#include <mangrove/nvp.hpp>
#include <mangrove/query_builder.hpp>

struct DataA : public mangrove::model<DataA> {
    int32_t x, y;
    double z;

    MANGROVE_MAKE_KEYS_MODEL(DataA, MANGROVE_NVP(x), MANGROVE_NVP(y), MANGROVE_NVP(z))

    bool operator==(const DataA& other) {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }

    bsoncxx::oid getID() {
        return _id;
    }
};

TEST_CASE("the model base class successfully allows the saving and removing of records.",
          "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataA::setCollection(db["data_a"]);
    DataA::drop();

    DataA a1;

    a1.x = 16;
    a1.y = 32;
    a1.z = 1.50;

    a1.save();

    auto query_filter = bsoncxx::builder::stream::document{} << "x" << 16
                                                             << bsoncxx::builder::stream::finalize;

    auto query_result = DataA::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(a1 == *query_result);

    query_result->remove();

    REQUIRE(!DataA::find_one(query_filter.view()));
}

TEST_CASE(
    "the model base class successfully provides a cursor to deserialized objects that can be "
    "subsequently updated.",
    "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_cursor_test"];
    DataA::setCollection(db["data_a"]);
    DataA::drop();

    for (int i = 0; i < 10; ++i) {
        DataA a1;
        a1.x = a1.y = a1.z = i;
        a1.save();
    }

    // Negate all x values in the collection via pulling them in from a cursor.
    for (auto a : DataA::find({})) {
        a.x *= -1;
        a.save();
    }

    // Sum up the x values in the collection
    int sum = 0;
    for (auto a : DataA::find({})) {
        sum += a.x;
    }

    REQUIRE(sum == -45);
}

using boson::stdx::optional;

struct DataB : public mangrove::model<DataB> {
    int32_t x;
    optional<int32_t> y;
    optional<double> z;

    bool operator==(const DataB& other) {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(_id), CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }

    bsoncxx::oid getID() {
        return _id;
    }
};

TEST_CASE(
    "the model base class successfully allows dynamic schemas by way of allowing stdx::optional "
    "elements.",
    "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataB::setCollection(db["data_b"]);
    DataB::drop();

    DataB b1;

    b1.x = 16;
    b1.z = 1.50;

    b1.save();

    auto query_filter = bsoncxx::builder::stream::document{} << "x" << 16
                                                             << bsoncxx::builder::stream::finalize;

    auto query_result = DataB::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(!query_result->y);
    REQUIRE(b1 == *query_result);

    query_result->remove();

    REQUIRE(!DataB::find_one(query_filter.view()));
}

struct EmbeddedVals {
    int32_t x;
    double y;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(x), CEREAL_NVP(y));
    }

    bool operator==(const EmbeddedVals& other) {
        return std::tie(x, y) == std::tie(other.x, other.y);
    }
};

struct DataC : public mangrove::model<DataC> {
    int64_t a, b;
    EmbeddedVals m;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(_id), CEREAL_NVP(a), CEREAL_NVP(b), CEREAL_NVP(m));
    }

    bool operator==(const DataC& other) {
        return std::tie(a, b, m) == std::tie(other.a, other.b, other.m);
    }
};

// Struct with _id as std::string
struct DataD : public mangrove::model<DataD, std::string> {
    int32_t x, y;
    double z;

    DataD() {
    }

    DataD(const char* s) : mangrove::model<DataD, std::string>(s) {
    }

    bool operator==(const DataD& other) {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(_id), CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }

    std::string getID() {
        return _id;
    }
};

TEST_CASE(
    "the model base class successfully allows the saving and removing of records when _id is of a "
    "custom type.",
    "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataD::setCollection(db["data_d"]);
    DataD::drop();

    DataD d("my very first DataD");

    d.x = 16;
    d.y = 32;
    d.z = 1.50;

    d.save();

    auto query_filter = bsoncxx::builder::stream::document{} << "x" << 16
                                                             << bsoncxx::builder::stream::finalize;

    auto query_result = DataD::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(d == *query_result);

    query_result->remove();

    REQUIRE(!DataD::find_one(query_filter.view()));
}

TEST_CASE(
    "the model base class successfully allows the saving and removing of records with embedded "
    "documents.",
    "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataC::setCollection(db["data_c"]);
    DataC::drop();

    DataC c;

    c.a = 229;
    c.b = 43;
    c.m.x = 13;
    c.m.y = 1.50;

    c.save();

    auto query_filter = bsoncxx::builder::stream::document{} << "a" << 229
                                                             << bsoncxx::builder::stream::finalize;

    auto query_result = DataC::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(c == *query_result);

    query_result->remove();

    REQUIRE(!DataC::find_one(query_filter.view()));
}

struct OptEmbeddedVals {
    optional<int32_t> x;
    optional<double> y;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(x), CEREAL_NVP(y));
    }

    bool operator==(const OptEmbeddedVals& other) {
        return std::tie(x, y) == std::tie(other.x, other.y);
    }
};

struct DataE : public mangrove::model<DataE> {
    int64_t a, b;
    OptEmbeddedVals m;

    DataE() = default;

    DataE(bsoncxx::oid id) : mangrove::model<DataE>(id) {
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(_id), CEREAL_NVP(a), CEREAL_NVP(b), CEREAL_NVP(m));
    }

    bool operator==(const DataE& other) {
        return std::tie(a, b, m) == std::tie(other.a, other.b, other.m);
    }

    bsoncxx::oid getID() {
        return _id;
    }
};

TEST_CASE(
    "the model base class does not overwrite existing fields in embedded documents on save (when "
    "they are not in an array).",
    "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataE::setCollection(db["data_e"]);
    DataE::drop();

    DataE e_with_embedded_x;

    e_with_embedded_x.a = 229;
    e_with_embedded_x.b = 43;
    e_with_embedded_x.m.x = 13;

    e_with_embedded_x.save();

    auto query_filter = bsoncxx::builder::stream::document{} << "_id" << e_with_embedded_x.getID()
                                                             << bsoncxx::builder::stream::finalize;

    auto query_result = DataE::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(!query_result->m.y);
    REQUIRE(e_with_embedded_x == *query_result);

    DataE e_with_embedded_y(e_with_embedded_x.getID());
    e_with_embedded_y.a = 229;
    e_with_embedded_y.b = 43;
    e_with_embedded_y.m.y = 1.50;

    e_with_embedded_y.save();

    query_filter = bsoncxx::builder::stream::document{} << "_id" << e_with_embedded_y.getID()
                                                        << bsoncxx::builder::stream::finalize;

    query_result = DataE::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(query_result->m.x);
    REQUIRE(e_with_embedded_x.m.x == *query_result->m.x);

    REQUIRE(query_result->m.y);
    REQUIRE(e_with_embedded_y.m.y == *query_result->m.y);

    query_result->remove();

    REQUIRE(!DataE::find_one(query_filter.view()));
}

TEST_CASE("the model base class allows the counting of documents in the associated collection.",
          "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataA::setCollection(db["data_a"]);
    DataA::drop();

    REQUIRE(DataA::count() == 0);

    DataA x, y, z;

    x.save();
    y.save();
    z.save();

    REQUIRE(DataA::count() == 3);
}

TEST_CASE(
    "the model base class allows the bulk and single insertion and deletion of documents in the "
    "associated collection.",
    "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataA::setCollection(db["data_a"]);
    DataA::drop();

    std::vector<DataA> data(10);

    DataA::insert_many(data);

    REQUIRE(DataA::count() == 10);

    DataA single;
    single.x = 1;
    single.y = 2;
    single.z = 3.0;
    DataA::insert_one(single);

    REQUIRE(DataA::count() == 11);

    // Should not compile:
    //     DataB other_data(10);
    //     DataA::insert_many(other_data);
    //     DataB other_single;
    //     DataA::insert_one(other_single)

    DataA::delete_one(MANGROVE_KEY(DataA::x) == 1);

    REQUIRE(DataA::count() == 10);

    DataA::delete_many({});

    REQUIRE(DataA::count() == 0);
}

TEST_CASE(
    "the model base class allows bulk and single updates of documents in the associated "
    "collection.",
    "[mangrove::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mangrove_model_test"];

    DataA::setCollection(db["data_a"]);
    DataA::drop();

    DataA single;
    single.x = 1;
    single.y = 2;
    single.z = 3.0;
    single.save();

    REQUIRE(DataA::find_one(MANGROVE_KEY(DataA::x) == 1));

    DataA::update_one(MANGROVE_KEY(DataA::x) == 1, MANGROVE_KEY(DataA::x) = 10);

    REQUIRE(!DataA::find_one(MANGROVE_KEY(DataA::x) == 1));
    REQUIRE(DataA::find_one(MANGROVE_KEY(DataA::x) == 10));

    DataA other;
    other.x = 10;
    other.y = 5;
    other.z = 4.0;
    other.save();

    REQUIRE(DataA::count(MANGROVE_KEY(DataA::x) == 10) == 2);

    DataA::update_many(MANGROVE_KEY(DataA::x) == 10, MANGROVE_KEY(DataA::y) = 229);

    REQUIRE(DataA::count(MANGROVE_KEY(DataA::y) == 229) == 2);
}
