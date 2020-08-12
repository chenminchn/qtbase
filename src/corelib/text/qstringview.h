/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Copyright (C) 2019 Mail.ru Group.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QSTRINGVIEW_H
#define QSTRINGVIEW_H

/*
    This macro enables three "levels" of QStringView support:

    1. offer QStringView, overload some functions taking QString with
    QStringView

    2. like 1, but remove all overloads of functions taking QStringRef,
    leaving only the function taking QStringView.  Do this only where
    QStringRef overloads tradionally existed.

    3. like 2, but replace functions taking QString, too.
*/
#ifndef QT_STRINGVIEW_LEVEL
#  define QT_STRINGVIEW_LEVEL 1
#endif

#include <QtCore/qchar.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstringliteral.h>
#include <QtCore/qstringalgorithms.h>

#include <string>

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
Q_FORWARD_DECLARE_CF_TYPE(CFString);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);
#endif

QT_BEGIN_NAMESPACE

class QString;
class QStringRef;
class QStringView;
class QRegularExpression;

namespace QtPrivate {
template <typename Char>
struct IsCompatibleCharTypeHelper
    : std::integral_constant<bool,
                             std::is_same<Char, QChar>::value ||
                             std::is_same<Char, ushort>::value ||
                             std::is_same<Char, char16_t>::value ||
                             (std::is_same<Char, wchar_t>::value && sizeof(wchar_t) == sizeof(QChar))> {};
template <typename Char>
struct IsCompatibleCharType
    : IsCompatibleCharTypeHelper<typename std::remove_cv<typename std::remove_reference<Char>::type>::type> {};

template <typename Pointer>
struct IsCompatiblePointerHelper : std::false_type {};
template <typename Char>
struct IsCompatiblePointerHelper<Char*>
    : IsCompatibleCharType<Char> {};
template <typename Pointer>
struct IsCompatiblePointer
    : IsCompatiblePointerHelper<typename std::remove_cv<typename std::remove_reference<Pointer>::type>::type> {};

template <typename T, typename Enable = void>
struct IsContainerCompatibleWithQStringView : std::false_type {};

template <typename T>
struct IsContainerCompatibleWithQStringView<T, std::enable_if_t<std::conjunction_v<
            // lacking concepts and ranges, we accept any T whose std::data yields a suitable pointer ...
            IsCompatiblePointer<decltype( std::data(std::declval<const T &>()) )>,
            // ... and that has a suitable size ...
            std::is_convertible<decltype( std::size(std::declval<const T &>()) ), qsizetype>,
            // ... and it's a range as it defines an iterator-like API
            IsCompatibleCharType<typename std::iterator_traits<decltype( std::begin(std::declval<const T &>()) )>::value_type>,
            std::is_convertible<
                decltype( std::begin(std::declval<const T &>()) != std::end(std::declval<const T &>()) ),
                bool>,

            // These need to be treated specially due to the empty vs null distinction
            std::negation<std::is_same<std::decay_t<T>, QString>>,
            std::negation<std::is_same<std::decay_t<T>, QStringRef>>,

            // Don't make an accidental copy constructor
            std::negation<std::is_same<std::decay_t<T>, QStringView>>
        >>> : std::true_type {};

} // namespace QtPrivate

class QStringView
{
public:
    typedef char16_t storage_type;
    typedef const QChar value_type;
    typedef std::ptrdiff_t difference_type;
    typedef qsizetype size_type;
    typedef value_type &reference;
    typedef value_type &const_reference;
    typedef value_type *pointer;
    typedef value_type *const_pointer;

    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
    template <typename Char>
    using if_compatible_char = typename std::enable_if<QtPrivate::IsCompatibleCharType<Char>::value, bool>::type;

    template <typename Pointer>
    using if_compatible_pointer = typename std::enable_if<QtPrivate::IsCompatiblePointer<Pointer>::value, bool>::type;

    template <typename T>
    using if_compatible_qstring_like = typename std::enable_if<std::is_same<T, QString>::value || std::is_same<T, QStringRef>::value, bool>::type;

    template <typename T>
    using if_compatible_container = typename std::enable_if<QtPrivate::IsContainerCompatibleWithQStringView<T>::value, bool>::type;

    template <typename Char>
    static qsizetype lengthHelperPointer(const Char *str) noexcept
    {
#if defined(Q_CC_GNU) && !defined(Q_CC_CLANG) && !defined(Q_CC_INTEL)
        if (__builtin_constant_p(*str)) {
            qsizetype result = 0;
            while (*str++)
                ++result;
            return result;
        }
#endif
        return QtPrivate::qustrlen(reinterpret_cast<const char16_t *>(str));
    }
    static qsizetype lengthHelperPointer(const QChar *str) noexcept
    {
        return QtPrivate::qustrlen(reinterpret_cast<const char16_t *>(str));
    }

    template <typename Container>
    static constexpr qsizetype lengthHelperContainer(const Container &c) noexcept
    {
        return qsizetype(std::size(c));
    }

    template <typename Char, size_t N>
    static constexpr qsizetype lengthHelperContainer(const Char (&)[N]) noexcept
    {
        return qsizetype(N - 1);
    }

    template <typename Char>
    static const storage_type *castHelper(const Char *str) noexcept
    { return reinterpret_cast<const storage_type*>(str); }
    static constexpr const storage_type *castHelper(const storage_type *str) noexcept
    { return str; }

public:
    constexpr QStringView() noexcept
        : m_size(0), m_data(nullptr) {}
    constexpr QStringView(std::nullptr_t) noexcept
        : QStringView() {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QStringView(const Char *str, qsizetype len)
        : m_size((Q_ASSERT(len >= 0), Q_ASSERT(str || !len), len)),
          m_data(castHelper(str)) {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QStringView(const Char *f, const Char *l)
        : QStringView(f, l - f) {}

#ifdef Q_CLANG_QDOC
    template <typename Char, size_t N>
    constexpr QStringView(const Char (&array)[N]) noexcept;

    template <typename Char>
    constexpr QStringView(const Char *str) noexcept;
#else

    template <typename Pointer, if_compatible_pointer<Pointer> = true>
    constexpr QStringView(const Pointer &str) noexcept
        : QStringView(str, str ? lengthHelperPointer(str) : 0) {}
#endif

#ifdef Q_CLANG_QDOC
    QStringView(const QString &str) noexcept;
    QStringView(const QStringRef &str) noexcept;
#else
    template <typename String, if_compatible_qstring_like<String> = true>
    QStringView(const String &str) noexcept
        : QStringView(str.isNull() ? nullptr : str.data(), qsizetype(str.size())) {}
#endif

    template <typename Container, if_compatible_container<Container> = true>
    constexpr QStringView(const Container &c) noexcept
        : QStringView(std::data(c), lengthHelperContainer(c)) {}

    Q_REQUIRED_RESULT inline QString toString() const; // defined in qstring.h
#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    // defined in qcore_foundation.mm
    Q_REQUIRED_RESULT Q_CORE_EXPORT CFStringRef toCFString() const Q_DECL_CF_RETURNS_RETAINED;
    Q_REQUIRED_RESULT Q_CORE_EXPORT NSString *toNSString() const Q_DECL_NS_RETURNS_AUTORELEASED;
#endif

    Q_REQUIRED_RESULT constexpr qsizetype size() const noexcept { return m_size; }
    Q_REQUIRED_RESULT const_pointer data() const noexcept { return reinterpret_cast<const_pointer>(m_data); }
    Q_REQUIRED_RESULT const_pointer constData() const noexcept { return data(); }
    Q_REQUIRED_RESULT constexpr const storage_type *utf16() const noexcept { return m_data; }

    Q_REQUIRED_RESULT constexpr QChar operator[](qsizetype n) const
    { return Q_ASSERT(n >= 0), Q_ASSERT(n < size()), QChar(m_data[n]); }

    //
    // QString API
    //

    template <typename...Args>
    Q_REQUIRED_RESULT inline QString arg(Args &&...args) const; // defined in qstring.h

    Q_REQUIRED_RESULT QByteArray toLatin1() const { return QtPrivate::convertToLatin1(*this); }
    Q_REQUIRED_RESULT QByteArray toUtf8() const { return QtPrivate::convertToUtf8(*this); }
    Q_REQUIRED_RESULT QByteArray toLocal8Bit() const { return QtPrivate::convertToLocal8Bit(*this); }
    Q_REQUIRED_RESULT inline QList<uint> toUcs4() const; // defined in qlist.h

    Q_REQUIRED_RESULT constexpr QChar at(qsizetype n) const { return (*this)[n]; }

    Q_REQUIRED_RESULT constexpr QStringView mid(qsizetype pos, qsizetype n = -1) const
    {
        using namespace QtPrivate;
        auto result = QContainerImplHelper::mid(size(), &pos, &n);
        return result == QContainerImplHelper::Null ? QStringView() : QStringView(m_data + pos, n);
    }
    Q_REQUIRED_RESULT constexpr QStringView left(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return QStringView(m_data, n);
    }
    Q_REQUIRED_RESULT constexpr QStringView right(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return QStringView(m_data + m_size - n, n);
    }

    Q_REQUIRED_RESULT constexpr QStringView first(qsizetype n) const
    { Q_ASSERT(n >= 0); Q_ASSERT(n <= size()); return QStringView(m_data, n); }
    Q_REQUIRED_RESULT constexpr QStringView last(qsizetype n) const
    { Q_ASSERT(n >= 0); Q_ASSERT(n <= size()); return QStringView(m_data + size() - n, n); }
    Q_REQUIRED_RESULT constexpr QStringView sliced(qsizetype pos) const
    { Q_ASSERT(pos >= 0); Q_ASSERT(pos <= size()); return QStringView(m_data + pos, size() - pos); }
    Q_REQUIRED_RESULT constexpr QStringView sliced(qsizetype pos, qsizetype n) const
    { Q_ASSERT(pos >= 0); Q_ASSERT(n >= 0); Q_ASSERT(size_t(pos) + size_t(n) <= size_t(size())); return QStringView(m_data + pos, n); }
    Q_REQUIRED_RESULT constexpr QStringView chopped(qsizetype n) const
    { return Q_ASSERT(n >= 0), Q_ASSERT(n <= size()), QStringView(m_data, m_size - n); }

    constexpr void truncate(qsizetype n)
    { Q_ASSERT(n >= 0); Q_ASSERT(n <= size()); m_size = n; }
    constexpr void chop(qsizetype n)
    { Q_ASSERT(n >= 0); Q_ASSERT(n <= size()); m_size -= n; }

    Q_REQUIRED_RESULT QStringView trimmed() const noexcept { return QtPrivate::trimmed(*this); }

    template <typename Needle, typename...Flags>
    Q_REQUIRED_RESULT constexpr inline auto tokenize(Needle &&needle, Flags...flags) const
        noexcept(noexcept(qTokenize(std::declval<const QStringView&>(), std::forward<Needle>(needle), flags...)))
            -> decltype(qTokenize(*this, std::forward<Needle>(needle), flags...))
    { return qTokenize(*this, std::forward<Needle>(needle), flags...); }

    Q_REQUIRED_RESULT int compare(QStringView other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::compareStrings(*this, other, cs); }
    Q_REQUIRED_RESULT inline int compare(QLatin1String other, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    Q_REQUIRED_RESULT constexpr int compare(QChar c) const noexcept
    { return size() >= 1 ? compare_single_char_helper(*utf16() - c.unicode()) : -1; }
    Q_REQUIRED_RESULT int compare(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::compareStrings(*this, QStringView(&c, 1), cs); }

    Q_REQUIRED_RESULT bool startsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::startsWith(*this, s, cs); }
    Q_REQUIRED_RESULT inline bool startsWith(QLatin1String s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    Q_REQUIRED_RESULT bool startsWith(QChar c) const noexcept
    { return !empty() && front() == c; }
    Q_REQUIRED_RESULT bool startsWith(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::startsWith(*this, QStringView(&c, 1), cs); }

    Q_REQUIRED_RESULT bool endsWith(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::endsWith(*this, s, cs); }
    Q_REQUIRED_RESULT inline bool endsWith(QLatin1String s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;
    Q_REQUIRED_RESULT bool endsWith(QChar c) const noexcept
    { return !empty() && back() == c; }
    Q_REQUIRED_RESULT bool endsWith(QChar c, Qt::CaseSensitivity cs) const noexcept
    { return QtPrivate::endsWith(*this, QStringView(&c, 1), cs); }

    Q_REQUIRED_RESULT qsizetype indexOf(QChar c, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, QStringView(&c, 1), cs); }
    Q_REQUIRED_RESULT qsizetype indexOf(QStringView s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::findString(*this, from, s, cs); }
    Q_REQUIRED_RESULT inline qsizetype indexOf(QLatin1String s, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

    Q_REQUIRED_RESULT bool contains(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(QStringView(&c, 1), 0, cs) != qsizetype(-1); }
    Q_REQUIRED_RESULT bool contains(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(s, 0, cs) != qsizetype(-1); }
    Q_REQUIRED_RESULT inline bool contains(QLatin1String s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

    Q_REQUIRED_RESULT qsizetype count(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::count(*this, c, cs); }
    Q_REQUIRED_RESULT qsizetype count(QStringView s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::count(*this, s, cs); }

    Q_REQUIRED_RESULT qsizetype lastIndexOf(QChar c, qsizetype from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, QStringView(&c, 1), cs); }
    Q_REQUIRED_RESULT qsizetype lastIndexOf(QStringView s, qsizetype from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::lastIndexOf(*this, from, s, cs); }
    Q_REQUIRED_RESULT inline qsizetype lastIndexOf(QLatin1String s, qsizetype from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept;

    Q_REQUIRED_RESULT bool isRightToLeft() const noexcept
    { return QtPrivate::isRightToLeft(*this); }
    Q_REQUIRED_RESULT bool isValidUtf16() const noexcept
    { return QtPrivate::isValidUtf16(*this); }

    Q_REQUIRED_RESULT inline short toShort(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT inline ushort toUShort(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT inline int toInt(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT inline uint toUInt(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT inline long toLong(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT inline ulong toULong(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT inline qlonglong toLongLong(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT inline qulonglong toULongLong(bool *ok = nullptr, int base = 10) const;
    Q_REQUIRED_RESULT Q_CORE_EXPORT float toFloat(bool *ok = nullptr) const;
    Q_REQUIRED_RESULT Q_CORE_EXPORT double toDouble(bool *ok = nullptr) const;

    Q_REQUIRED_RESULT inline int toWCharArray(wchar_t *array) const; // defined in qstring.h


    Q_REQUIRED_RESULT Q_CORE_EXPORT
    QList<QStringView> split(QStringView sep,
                             Qt::SplitBehavior behavior = Qt::KeepEmptyParts,
                             Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    Q_REQUIRED_RESULT Q_CORE_EXPORT
    QList<QStringView> split(QChar sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts,
                             Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

#if QT_CONFIG(regularexpression)
    Q_REQUIRED_RESULT Q_CORE_EXPORT
    QList<QStringView> split(const QRegularExpression &sep,
                             Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
#endif

    //
    // STL compatibility API:
    //
    Q_REQUIRED_RESULT const_iterator begin()   const noexcept { return data(); }
    Q_REQUIRED_RESULT const_iterator end()     const noexcept { return data() + size(); }
    Q_REQUIRED_RESULT const_iterator cbegin()  const noexcept { return begin(); }
    Q_REQUIRED_RESULT const_iterator cend()    const noexcept { return end(); }
    Q_REQUIRED_RESULT const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    Q_REQUIRED_RESULT const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    Q_REQUIRED_RESULT const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    Q_REQUIRED_RESULT const_reverse_iterator crend()   const noexcept { return rend(); }

    Q_REQUIRED_RESULT constexpr bool empty() const noexcept { return size() == 0; }
    Q_REQUIRED_RESULT constexpr QChar front() const { return Q_ASSERT(!empty()), QChar(m_data[0]); }
    Q_REQUIRED_RESULT constexpr QChar back()  const { return Q_ASSERT(!empty()), QChar(m_data[m_size - 1]); }

    //
    // Qt compatibility API:
    //
    Q_REQUIRED_RESULT constexpr bool isNull() const noexcept { return !m_data; }
    Q_REQUIRED_RESULT constexpr bool isEmpty() const noexcept { return empty(); }
    Q_REQUIRED_RESULT constexpr int length() const /* not nothrow! */
    { return Q_ASSERT(int(size()) == size()), int(size()); }
    Q_REQUIRED_RESULT constexpr QChar first() const { return front(); }
    Q_REQUIRED_RESULT constexpr QChar last()  const { return back(); }
private:
    qsizetype m_size;
    const storage_type *m_data;

    constexpr int compare_single_char_helper(int diff) const noexcept
    { return diff ? diff : size() > 1 ? 1 : 0; }
};
Q_DECLARE_TYPEINFO(QStringView, Q_PRIMITIVE_TYPE);

template <typename QStringLike, typename std::enable_if<
    std::is_same<QStringLike, QString>::value || std::is_same<QStringLike, QStringRef>::value,
    bool>::type = true>
inline QStringView qToStringViewIgnoringNull(const QStringLike &s) noexcept
{ return QStringView(s.data(), s.size()); }

// QChar inline functions:

Q_REQUIRED_RESULT constexpr auto QChar::fromUcs4(char32_t c) noexcept
{
    struct R {
        char16_t chars[2];
        Q_REQUIRED_RESULT constexpr operator QStringView() const noexcept { return {begin(), end()}; }
        Q_REQUIRED_RESULT constexpr qsizetype size() const noexcept { return chars[1] ? 2 : 1; }
        Q_REQUIRED_RESULT constexpr const char16_t *begin() const noexcept { return chars; }
        Q_REQUIRED_RESULT constexpr const char16_t *end() const noexcept { return begin() + size(); }
    };
    return requiresSurrogates(c) ? R{{QChar::highSurrogate(c),
                                      QChar::lowSurrogate(c)}} :
                                   R{{char16_t(c), u'\0'}} ;
}

QT_END_NAMESPACE

#endif /* QSTRINGVIEW_H */
