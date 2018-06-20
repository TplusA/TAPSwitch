/*
 * Copyright (C) 2017, 2018  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of TAPSwitch.
 *
 * TAPSwitch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3 as
 * published by the Free Software Foundation.
 *
 * TAPSwitch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TAPSwitch.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <cppcutter.h>
#include <string>

#include "mock_audiopath_dbus.hh"

enum class AudiopathFn
{
    player_activate,
    player_deactivate,
    source_selected_on_hold,
    source_selected,
    source_deselected,

    first_valid_audiopath_fn_id = player_activate,
    last_valid_audiopath_fn_id = source_deselected,
};

static std::ostream &operator<<(std::ostream &os, const AudiopathFn id)
{
    if(id < AudiopathFn::first_valid_audiopath_fn_id ||
       id > AudiopathFn::last_valid_audiopath_fn_id)
    {
        os << "INVALID";
        return os;
    }

    switch(id)
    {
      case AudiopathFn::player_activate:
        os << "player_activate";
        break;

      case AudiopathFn::player_deactivate:
        os << "player_deactivate";
        break;

      case AudiopathFn::source_selected_on_hold:
        os << "source_selected_on_hold";
        break;

      case AudiopathFn::source_selected:
        os << "source_selected";
        break;

      case AudiopathFn::source_deselected:
        os << "source_deselected";
        break;
    }

    os << "()";

    return os;
}

class MockAudiopathDBus::Expectation
{
  public:
    struct Data
    {
        const AudiopathFn function_id_;

        bool ret_bool_;
        void *arg_object_;
        std::string arg_source_id_;
        GVariantWrapper arg_request_data_;

        explicit Data(AudiopathFn fn):
            function_id_(fn),
            ret_bool_(false),
            arg_object_(nullptr)
        {}
    };

    const Data d;

  private:
    /* writable reference for simple ctor code */
    Data &data_ = *const_cast<Data *>(&d);

  public:
    Expectation(const Expectation &) = delete;
    Expectation &operator=(const Expectation &) = delete;

    explicit Expectation(AudiopathFn fn, bool retval, tdbusaupathPlayer *object):
        d(fn)
    {
        data_.ret_bool_ = retval;
        data_.arg_object_ = static_cast<void *>(object);
    }

    explicit Expectation(AudiopathFn fn, bool retval, tdbusaupathPlayer *object,
                         GVariantWrapper &&request_data):
        d(fn)
    {
        data_.ret_bool_ = retval;
        data_.arg_object_ = static_cast<void *>(object);
        data_.arg_request_data_ = std::move(request_data);
    }

    explicit Expectation(AudiopathFn fn, bool retval, tdbusaupathSource *object,
                         const char *source_id):
        d(fn)
    {
        data_.ret_bool_ = retval;
        data_.arg_object_ = static_cast<void *>(object);
        data_.arg_source_id_ = source_id;
    }

    explicit Expectation(AudiopathFn fn, bool retval, tdbusaupathSource *object,
                         const char *source_id, GVariantWrapper &&request_data):
        d(fn)
    {
        data_.ret_bool_ = retval;
        data_.arg_object_ = static_cast<void *>(object);
        data_.arg_source_id_ = source_id;
        data_.arg_request_data_ = std::move(request_data);
    }

    Expectation(Expectation &&) = default;
};


MockAudiopathDBus::MockAudiopathDBus()
{
    expectations_ = new MockExpectations();
}

MockAudiopathDBus::~MockAudiopathDBus()
{
    delete expectations_;
}

void MockAudiopathDBus::init()
{
    cppcut_assert_not_null(expectations_);
    expectations_->init();
}

void MockAudiopathDBus::check() const
{
    cppcut_assert_not_null(expectations_);
    expectations_->check();
}

void MockAudiopathDBus::expect_tdbus_aupath_player_call_activate_sync(gboolean retval, tdbusaupathPlayer *object)
{
    expectations_->add(Expectation(AudiopathFn::player_activate,
                                   retval, object));
}

void MockAudiopathDBus::expect_tdbus_aupath_player_call_activate_sync(gboolean retval, tdbusaupathPlayer *object, GVariantWrapper &&request_data)
{
    expectations_->add(Expectation(AudiopathFn::player_activate,
                                   retval, object, std::move(request_data)));
}

void MockAudiopathDBus::expect_tdbus_aupath_player_call_deactivate_sync(gboolean retval, tdbusaupathPlayer *object)
{
    expectations_->add(Expectation(AudiopathFn::player_deactivate,
                                   retval, object));
}

void MockAudiopathDBus::expect_tdbus_aupath_player_call_deactivate_sync(gboolean retval, tdbusaupathPlayer *object, GVariantWrapper &&request_data)
{
    expectations_->add(Expectation(AudiopathFn::player_deactivate,
                                   retval, object, std::move(request_data)));
}

void MockAudiopathDBus::expect_tdbus_aupath_source_call_selected_on_hold_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id)
{
    expectations_->add(Expectation(AudiopathFn::source_selected_on_hold,
                                   retval, object, arg_source_id));
}

void MockAudiopathDBus::expect_tdbus_aupath_source_call_selected_on_hold_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id, GVariantWrapper &&request_data)
{
    expectations_->add(Expectation(AudiopathFn::source_selected_on_hold,
                                   retval, object, arg_source_id, std::move(request_data)));
}

void MockAudiopathDBus::expect_tdbus_aupath_source_call_selected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id)
{
    expectations_->add(Expectation(AudiopathFn::source_selected,
                                   retval, object, arg_source_id));
}

void MockAudiopathDBus::expect_tdbus_aupath_source_call_selected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id, GVariantWrapper &&request_data)
{
    expectations_->add(Expectation(AudiopathFn::source_selected,
                                   retval, object, arg_source_id, std::move(request_data)));
}

void MockAudiopathDBus::expect_tdbus_aupath_source_call_deselected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id)
{
    expectations_->add(Expectation(AudiopathFn::source_deselected,
                                   retval, object, arg_source_id));
}

void MockAudiopathDBus::expect_tdbus_aupath_source_call_deselected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id, GVariantWrapper &&request_data)
{
    expectations_->add(Expectation(AudiopathFn::source_deselected,
                                   retval, object, arg_source_id, std::move(request_data)));
}


MockAudiopathDBus *mock_audiopath_dbus_singleton = nullptr;

static char extract_id(tdbusaupathPlayer *proxy)
{
    return reinterpret_cast<unsigned long>(proxy) & UINT8_MAX;
}

static char extract_id(tdbusaupathSource *proxy)
{
    return reinterpret_cast<unsigned long>(proxy) & UINT8_MAX;
}

static void check_request_data(const GVariantWrapper &expected_data,
                               GVariant *const arg_request_data,
                               bool auto_unref_data = true)
{
    cppcut_assert_not_null(arg_request_data);
    cut_assert_true(g_variant_is_of_type(arg_request_data, G_VARIANT_TYPE_VARDICT));

    g_variant_ref_sink(arg_request_data);

    if(expected_data != nullptr)
    {
        if(GVariantWrapper::get(expected_data) != arg_request_data)
            cut_assert_true(g_variant_equal(GVariantWrapper::get(expected_data),
                                            arg_request_data));
    }
    else
        cppcut_assert_equal(gsize(0), g_variant_n_children(arg_request_data));

    if(auto_unref_data)
        g_variant_unref(arg_request_data);
}

gboolean tdbus_aupath_player_call_activate_sync(tdbusaupathPlayer *proxy, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    const auto &expect(mock_audiopath_dbus_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, AudiopathFn::player_activate);
    cppcut_assert_equal(expect.d.arg_object_, static_cast<void *>(proxy));
    check_request_data(expect.d.arg_request_data_, arg_request_data);

    if(error != nullptr)
    {
        if(expect.d.ret_bool_)
            *error = nullptr;
        else
            *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "Mock player %c activation failure",
                                 extract_id(proxy));
    }

    return expect.d.ret_bool_;
}

gboolean tdbus_aupath_player_call_deactivate_sync(tdbusaupathPlayer *proxy, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    const auto &expect(mock_audiopath_dbus_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, AudiopathFn::player_deactivate);
    cppcut_assert_equal(expect.d.arg_object_, static_cast<void *>(proxy));
    check_request_data(expect.d.arg_request_data_, arg_request_data);

    if(error != nullptr)
    {
        if(expect.d.ret_bool_)
            *error = nullptr;
        else
            *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "Mock player %c deactivation failure",
                                 extract_id(proxy));
    }

    return expect.d.ret_bool_;
}

gboolean tdbus_aupath_source_call_selected_on_hold_sync(tdbusaupathSource *proxy, const gchar *arg_source_id, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    const auto &expect(mock_audiopath_dbus_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, AudiopathFn::source_selected_on_hold);
    cppcut_assert_equal(expect.d.arg_object_, static_cast<void *>(proxy));
    cppcut_assert_not_null(arg_source_id);
    cppcut_assert_equal(expect.d.arg_source_id_.c_str(), arg_source_id);
    check_request_data(expect.d.arg_request_data_, arg_request_data);

    if(error != nullptr)
    {
        if(expect.d.ret_bool_)
            *error = nullptr;
        else
            *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "Mock source %c deferred selection failure",
                                 extract_id(proxy));
    }

    return expect.d.ret_bool_;
}

gboolean tdbus_aupath_source_call_selected_sync(tdbusaupathSource *proxy, const gchar *arg_source_id, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    const auto &expect(mock_audiopath_dbus_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, AudiopathFn::source_selected);
    cppcut_assert_equal(expect.d.arg_object_, static_cast<void *>(proxy));
    cppcut_assert_not_null(arg_source_id);
    cppcut_assert_equal(expect.d.arg_source_id_.c_str(), arg_source_id);
    check_request_data(expect.d.arg_request_data_, arg_request_data);

    if(error != nullptr)
    {
        if(expect.d.ret_bool_)
            *error = nullptr;
        else
            *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "Mock source %c selection failure",
                                 extract_id(proxy));
    }

    return expect.d.ret_bool_;
}

gboolean tdbus_aupath_source_call_deselected_sync(tdbusaupathSource *proxy, const gchar *arg_source_id, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    const auto &expect(mock_audiopath_dbus_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, AudiopathFn::source_deselected);
    cppcut_assert_equal(expect.d.arg_object_, static_cast<void *>(proxy));
    cppcut_assert_not_null(arg_source_id);
    cppcut_assert_equal(expect.d.arg_source_id_.c_str(), arg_source_id);
    check_request_data(expect.d.arg_request_data_, arg_request_data);

    if(error != nullptr)
    {
        if(expect.d.ret_bool_)
            *error = nullptr;
        else
            *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "Mock source %c deselection failure",
                                 extract_id(proxy));
    }

    return expect.d.ret_bool_;
}
