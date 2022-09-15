/*
 * Copyright (C) 2017, 2018, 2020, 2022  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of TAPSwitch.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef MOCK_AUDIOPATH_DBUS_CC
#define MOCK_AUDIOPATH_DBUS_CC

#include "de_tahifi_audiopath.h"
#include "gvariantwrapper.hh"
#include "mock_expectation.hh"

namespace MockAudiopathDBus
{

/*! Base class for expectations. */
class Expectation
{
  private:
    std::string name_;
    unsigned int sequence_serial_;


  public:
    Expectation(const Expectation &) = delete;
    Expectation(Expectation &&) = default;
    Expectation &operator=(const Expectation &) = delete;
    Expectation &operator=(Expectation &&) = default;
    Expectation() = default;
    Expectation(std::string &&name):
        name_(std::move(name)),
        sequence_serial_(std::numeric_limits<unsigned int>::max())
    {}
    virtual ~Expectation() = default;
    const std::string &get_name() const { return name_; }
    void set_sequence_serial(unsigned int ss) { sequence_serial_ = ss; }
    unsigned int get_sequence_serial() const { return sequence_serial_; }
};

class Mock
{
  private:
    MockExpectationsTemplate<Expectation> expectations_;

  public:
    Mock(const Mock &) = delete;
    Mock &operator=(const Mock &) = delete;

    explicit Mock():
        expectations_("MockAudiopathDBus")
    {}

    void expect(std::unique_ptr<Expectation> expectation)
    {
        expectations_.add(std::move(expectation));
    }

    void expect(Expectation *expectation)
    {
        expectations_.add(std::unique_ptr<Expectation>(expectation));
    }

    template <typename T>
    void ignore(std::unique_ptr<T> default_result)
    {
        expectations_.ignore<T>(std::move(default_result));
    }

    template <typename T>
    void ignore(T *default_result)
    {
        expectations_.ignore<T>(std::unique_ptr<Expectation>(default_result));
    }

    template <typename T>
    void allow() { expectations_.allow<T>(); }

    void done() const { expectations_.done(); }

    template <typename T, typename ... Args>
    auto check_next(Args ... args) -> decltype(std::declval<T>().check(args...))
    {
        return expectations_.check_and_advance<T, decltype(std::declval<T>().check(args...))>(args...);
    }

    template <typename T>
    const T &next(const char *caller)
    {
        return expectations_.next<T>(caller);
    }
};


static inline char extract_id(tdbusaupathPlayer *proxy)
{
    return reinterpret_cast<unsigned long>(proxy) & UINT8_MAX;
}

static inline char extract_id(tdbusaupathSource *proxy)
{
    return reinterpret_cast<unsigned long>(proxy) & UINT8_MAX;
}

static void check_request_data(GVariant *const arg_request_data,
                               const GVariantWrapper &expected_data,
                               bool auto_unref_data = true)
{
    REQUIRE(arg_request_data != nullptr);
    CHECK(g_variant_is_of_type(arg_request_data, G_VARIANT_TYPE_VARDICT));

    g_variant_ref_sink(arg_request_data);

    if(expected_data != nullptr)
    {
        if(GVariantWrapper::get(expected_data) != arg_request_data)
            CHECK(g_variant_equal(GVariantWrapper::get(expected_data),
                                  arg_request_data));
    }
    else
        CHECK(g_variant_n_children(arg_request_data) == 0);

    if(auto_unref_data)
        g_variant_unref(arg_request_data);
}

class PlayerCall: public Expectation
{
  private:
    const bool retval_;
    tdbusaupathPlayer *const object_;
    GVariantWrapper request_data_;

  protected:
    explicit PlayerCall(gboolean retval, tdbusaupathPlayer *object,
                        GVariantWrapper &&request_data):
        retval_(retval),
        object_(object),
        request_data_(std::move(request_data))
    {}

    virtual ~PlayerCall() = default;

    bool check(tdbusaupathPlayer *proxy, GVariant *request_data) const
    {
        CHECK(proxy == object_);
        check_request_data(request_data, request_data_);
        return retval_;
    }
};

class PlayerActivateSync: public PlayerCall
{
  public:
    explicit PlayerActivateSync(gboolean retval, tdbusaupathPlayer *object):
        PlayerActivateSync(retval, object, GVariantWrapper())
    {}

    explicit PlayerActivateSync(gboolean retval, tdbusaupathPlayer *object,
                                GVariantWrapper &&request_data):
        PlayerCall(retval, object, std::move(request_data))
    {}

    virtual ~PlayerActivateSync() = default;

    bool check(tdbusaupathPlayer *proxy, GVariant *request_data,
               GCancellable *cancellable, GError **error) const
    {
        const bool result = PlayerCall::check(proxy, request_data);

        if(error != nullptr)
        {
            if(result)
                *error = nullptr;
            else
                *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "Mock player %c activation failure",
                                     extract_id(proxy));
        }

        return result;
    }
};

class PlayerDeactivateSync: public PlayerCall
{
  public:
    explicit PlayerDeactivateSync(gboolean retval, tdbusaupathPlayer *object):
        PlayerDeactivateSync(retval, object, GVariantWrapper())
    {}

    explicit PlayerDeactivateSync(gboolean retval, tdbusaupathPlayer *object,
                                  GVariantWrapper &&request_data):
        PlayerCall(retval, object, std::move(request_data))
    {}

    virtual ~PlayerDeactivateSync() = default;

    bool check(tdbusaupathPlayer *proxy, GVariant *request_data,
               GCancellable *cancellable, GError **error) const
    {
        const bool result = PlayerCall::check(proxy, request_data);

        if(error != nullptr)
        {
            if(result)
                *error = nullptr;
            else
                *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "Mock player %c deactivation failure",
                                     extract_id(proxy));
        }

        return result;
    }
};

class SourceCall: public Expectation
{
  private:
    const bool retval_;
    tdbusaupathSource *const object_;
    const std::string source_id_;
    GVariantWrapper request_data_;

  protected:
    explicit SourceCall(gboolean retval, tdbusaupathSource *object,
                        std::string &&source_id, GVariantWrapper &&request_data):
        retval_(retval),
        object_(object),
        source_id_(std::move(source_id)),
        request_data_(std::move(request_data))
    {}

    virtual ~SourceCall() = default;

    bool check(tdbusaupathSource *proxy, const char *source_id,
               GVariant *request_data) const
    {
        CHECK(proxy == object_);
        REQUIRE(source_id != nullptr);
        CHECK(source_id == source_id_);
        check_request_data(request_data, request_data_);
        return retval_;
    }
};

class SourceSelectedSync: public SourceCall
{
  public:
    explicit SourceSelectedSync(gboolean retval, tdbusaupathSource *object,
                                std::string &&source_id):
        SourceSelectedSync(retval, object, std::move(source_id), GVariantWrapper())
    {}

    explicit SourceSelectedSync(gboolean retval, tdbusaupathSource *object,
                                std::string &&source_id,
                                GVariantWrapper &&request_data):
        SourceCall(retval, object, std::move(source_id), std::move(request_data))
    {}

    virtual ~SourceSelectedSync() = default;

    bool check(tdbusaupathSource *proxy, const char *source_id,
               GVariant *request_data, GCancellable *cancellable,
               GError **error) const
    {
        const bool result = SourceCall::check(proxy, source_id, request_data);

        if(error != nullptr)
        {
            if(result)
                *error = nullptr;
            else
                *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "Mock source %c selection failure",
                                     extract_id(proxy));
        }

        return result;
    }
};

class SourceSelectedOnHoldSync: public SourceCall
{
  public:
    explicit SourceSelectedOnHoldSync(gboolean retval, tdbusaupathSource *object,
                                      std::string &&source_id):
        SourceSelectedOnHoldSync(retval, object, std::move(source_id),
                                 GVariantWrapper())
    {}

    explicit SourceSelectedOnHoldSync(gboolean retval, tdbusaupathSource *object,
                                      std::string &&source_id,
                                      GVariantWrapper &&request_data):
        SourceCall(retval, object, std::move(source_id), std::move(request_data))
    {}

    virtual ~SourceSelectedOnHoldSync() = default;

    bool check(tdbusaupathSource *proxy, const char *source_id,
               GVariant *request_data, GCancellable *cancellable,
               GError **error) const
    {
        const bool result = SourceCall::check(proxy, source_id, request_data);

        if(error != nullptr)
        {
            if(result)
                *error = nullptr;
            else
                *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "Mock source %c deferred selection failure",
                                     extract_id(proxy));
        }

        return result;
    }
};

class SourceDeselectedSync: public SourceCall
{
  public:
    explicit SourceDeselectedSync(gboolean retval, tdbusaupathSource *object,
                                  std::string &&source_id):
        SourceDeselectedSync(retval, object, std::move(source_id), GVariantWrapper())
    {}

    explicit SourceDeselectedSync(gboolean retval, tdbusaupathSource *object,
                                  std::string &&source_id,
                                  GVariantWrapper &&request_data):
        SourceCall(retval, object, std::move(source_id), std::move(request_data))
    {}

    virtual ~SourceDeselectedSync() = default;

    bool check(tdbusaupathSource *proxy, const char *source_id,
               GVariant *request_data, GCancellable *cancellable,
               GError **error) const
    {
        const bool result = SourceCall::check(proxy, source_id, request_data);

        if(error != nullptr)
        {
            if(result)
                *error = nullptr;
            else
                *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "Mock source %c deselection failure",
                                     extract_id(proxy));
        }

        return result;
    }
};

extern Mock *singleton;

}

#endif /* !MOCK_AUDIOPATH_DBUS_CC */
