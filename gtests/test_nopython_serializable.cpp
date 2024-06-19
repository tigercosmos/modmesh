#include <gtest/gtest.h>
#include <thread>

#ifdef Py_PYTHON_H
#error "Python.h should not be included."
#endif

#include <modmesh/SerializableItem.hpp>
namespace modmesh
{

namespace detail
{

struct Address : SerializableItem
{
    std::string country;
    std::string city;
    std::vector<std::string> phone_numbers;
    std::vector<int> zip_codes;

    MM_DECL_SERIALIZABLE(
        register_member("country", country);
        register_member("city", city);
        register_member("phone_numbers", phone_numbers);
        register_member("zip_codes", zip_codes);)
};

struct Pet : SerializableItem
{
    std::string name;
    int age;
    bool is_dog;
    bool is_cat;

    MM_DECL_SERIALIZABLE(
        register_member("name", name);
        register_member("age", age);
        register_member("is_dog", is_dog);
        register_member("is_cat", is_cat);)
};

struct Person : SerializableItem
{
    std::string name;
    int age;
    bool is_student;
    Address address;
    std::vector<Pet> pets;

    MM_DECL_SERIALIZABLE(
        register_member("name", name);
        register_member("age", age);
        register_member("is_student", is_student);
        register_member("address", address);
        register_member("pets", pets);)
};

Pet create_dog()
{
    Pet pet;
    pet.name = "Fluffy";
    pet.age = 3;
    pet.is_dog = true;
    pet.is_cat = false;
    return pet;
}

Pet create_cat()
{
    Pet pet;
    pet.name = "Whiskers";
    pet.age = 8;
    pet.is_dog = false;
    pet.is_cat = true;
    return pet;
}

Address create_address()
{
    Address address;
    address.country = "USA";
    address.city = "New York";
    address.phone_numbers = {"123-456-7890", "098-765-4321"};
    address.zip_codes = {10001, 10002};
    return address;
}

} // namespace detail

TEST(Json, serialization_simple)
{
    auto pet = detail::create_dog();
    std::string json = pet.to_json();
    EXPECT_EQ(json, "{\"name\":\"Fluffy\",\"age\":3,\"is_dog\":true,\"is_cat\":false}");
}

TEST(Json, serialization_wrong_order)
{
    auto pet = detail::create_dog();
    std::string json = pet.to_json();
    EXPECT_NE(json, "{\"name\":\"Fluffy\",\"age\":3,\"is_cat\":false,\"is_dog\":true\"}"); // cat and dog are swapped
}

TEST(Json, serialization_with_vector)
{
    auto address = detail::create_address();

    std::string json = address.to_json();
    EXPECT_EQ(json, "{\"country\":\"USA\",\"city\":\"New York\",\"phone_numbers\":[\"123-456-7890\",\"098-765-4321\"],\"zip_codes\":[10001,10002]}");
}

TEST(Json, serialization_with_object)
{

    detail::Person person;
    person.name = "John Doe";
    person.age = 30;
    person.is_student = true;
    person.address = detail::create_address();
    person.pets.push_back(detail::create_dog());
    person.pets.push_back(detail::create_cat());

    std::string json = person.to_json();

    std::string answer = std::string("{\"name\":\"John Doe\",\"age\":30,\"is_student\":true,") +
                         "\"address\":{\"country\":\"USA\",\"city\":\"New York\",\"phone_numbers\":[\"123-456-7890\",\"098-765-4321\"],\"zip_codes\":[10001,10002]}," +
                         "\"pets\":[{\"name\":\"Fluffy\",\"age\":3,\"is_dog\":true,\"is_cat\":false},{\"name\":\"Whiskers\",\"age\":8,\"is_dog\":false,\"is_cat\":true}]}";
}

TEST(json, deserialize_simple)
{
    std::string json = "{\"name\":\"Fluffy\",\"age\":3,\"is_dog\":true,\"is_cat\":false}";
    detail::Pet pet;
    pet.from_json(json);
    EXPECT_EQ(pet.name, "Fluffy");
    EXPECT_EQ(pet.age, 3);
    EXPECT_EQ(pet.is_dog, true);
    EXPECT_EQ(pet.is_cat, false);
}

TEST(json, deserialize_trim)
{
    std::string json = "{  \n \"name\" \t\t\t \t\n:\"Fluffy\", \t\n\"age\":3,\"is_dog\": \n\ttrue,   \"is_cat\":false\t\t}";
    detail::Pet pet;
    pet.from_json(json);
    EXPECT_EQ(pet.name, "Fluffy");
    EXPECT_EQ(pet.age, 3);
    EXPECT_EQ(pet.is_dog, true);
    EXPECT_EQ(pet.is_cat, false);
}

TEST(json, deserialize_with_vector)
{
    std::string json = "{\"country\":\"USA\",\"city\":\"New York\",\"phone_numbers\":[\"123-456-7890\",\"098-765-4321\"],\"zip_codes\":[10001,10002]}";
    detail::Address address;
    address.from_json(json);
    EXPECT_EQ(address.country, "USA");
    EXPECT_EQ(address.city, "New York");
    EXPECT_EQ(address.phone_numbers.size(), 2);
    EXPECT_EQ(address.phone_numbers[0], "123-456-7890");
    EXPECT_EQ(address.phone_numbers[1], "098-765-4321");
    EXPECT_EQ(address.zip_codes.size(), 2);
    EXPECT_EQ(address.zip_codes[0], 10001);
    EXPECT_EQ(address.zip_codes[1], 10002);
}

TEST(json, deserialize_with_object)
{
    std::string json = std::string("{\"name\":\"John Doe\",\"age\":30,\"is_student\":true,") +
                       "\"address\":{\"country\":\"USA\",\"city\":\"New York\",\"phone_numbers\":[\"123-456-7890\",\"098-765-4321\"],\"zip_codes\":[10001,10002]}," +
                       "\"pets\":[{\"name\":\"Fluffy\",\"age\":3,\"is_dog\":true,\"is_cat\":false},{\"name\":\"Whiskers\",\"age\":8,\"is_dog\":false,\"is_cat\":true}]}";

    detail::Person person;
    person.from_json(json);

    EXPECT_EQ(person.name, "John Doe");
    EXPECT_EQ(person.age, 30);
    EXPECT_EQ(person.is_student, true);

    EXPECT_EQ(person.address.country, "USA");
    EXPECT_EQ(person.address.city, "New York");
    EXPECT_EQ(person.address.phone_numbers.size(), 2);
    EXPECT_EQ(person.address.phone_numbers[0], "123-456-7890");
    EXPECT_EQ(person.address.phone_numbers[1], "098-765-4321");
    EXPECT_EQ(person.address.zip_codes.size(), 2);
    EXPECT_EQ(person.address.zip_codes[0], 10001);
    EXPECT_EQ(person.address.zip_codes[1], 10002);

    EXPECT_EQ(person.pets.size(), 2);
    EXPECT_EQ(person.pets[0].name, "Fluffy");
    EXPECT_EQ(person.pets[0].age, 3);
    EXPECT_EQ(person.pets[0].is_dog, true);
    EXPECT_EQ(person.pets[0].is_cat, false);

    EXPECT_EQ(person.pets[1].name, "Whiskers");
    EXPECT_EQ(person.pets[1].age, 8);
    EXPECT_EQ(person.pets[1].is_dog, false);
    EXPECT_EQ(person.pets[1].is_cat, true);
}

} // namespace modmesh