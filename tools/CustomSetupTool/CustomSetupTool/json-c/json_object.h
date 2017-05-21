/*
 * $Id: json_object.h,v 1.12 2006/01/30 23:07:57 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#ifndef _json_object_h_
#define _json_object_h_

#ifdef __GNUC__
#define THIS_FUNCTION_IS_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define THIS_FUNCTION_IS_DEPRECATED(func) __declspec(deprecated) func
#else
#define THIS_FUNCTION_IS_DEPRECATED(func) func
#endif

#include "json_inttypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JSON_OBJECT_DEF_HASH_ENTRIES 16

/**
 * A flag for the json_object_to_json_string_ext() and
 * json_object_to_file_ext() functions which causes the output
 * to have no extra whitespace or formatting applied.
 */
#define JSON_C_TO_STRING_PLAIN      0
/**
 * A flag for the json_object_to_json_string_ext() and
 * json_object_to_file_ext() functions which causes the output to have
 * minimal whitespace inserted to make things slightly more readable.
 */
#define JSON_C_TO_STRING_SPACED     (1<<0)
/**
 * A flag for the json_object_to_json_string_ext() and
 * json_object_to_file_ext() functions which causes
 * the output to be formatted.
 *
 * See the "Two Space Tab" option at http://jsonformatter.curiousconcept.com/
 * for an example of the format.
 */
#define JSON_C_TO_STRING_PRETTY     (1<<1)
/**
 * A flag to drop trailing zero for float values
 */
#define JSON_C_TO_STRING_NOZERO     (1<<2)

#undef FALSE
#define FALSE ((json_bool)0)

#undef TRUE
#define TRUE ((json_bool)1)

extern const char *json_number_chars;
extern const char *json_hex_chars;

/* CAW: added for ANSI C iteration correctness */
struct json_object_iter
{
    char *key;
    struct json_object *val;
    struct lh_entry *entry;
};

/* forward structure definitions */

typedef int json_bool;
typedef struct printbuf printbuf;
typedef struct lh_table lh_table;
typedef struct array_list array_list;
typedef struct json_object json_object, *json_object_ptr;
typedef struct json_object_iter json_object_iter;
typedef struct json_tokener json_tokener;

/**
 * Type of custom user delete functions.  See json_object_set_serializer.
 */
typedef void (json_object_delete_fn)(struct json_object *jso, void *userdata);

/**
 * Type of a custom serialization function.  See json_object_set_serializer.
 */
typedef int (json_object_to_json_string_fn)(struct json_object *jso,
                        struct printbuf *pb,
                        int level,
                        int flags);

/* supported object types */

typedef enum json_type {
  /* If you change this, be sure to update json_type_to_name() too */
  json_type_null,
  json_type_boolean,
  json_type_double,
  json_type_int,
  json_type_object,
  json_type_array,
  json_type_string,
} json_type;

/* reference counting functions */

/**
 * Increment the reference count of json_object, thereby grabbing shared 
 * ownership of obj.
 *
 * @param obj the json_object instance
 */
extern struct json_object* json_object_get(struct json_object *obj);

/**
 * Decrement the reference count of json_object and free if it reaches zero.
 * You must have ownership of obj prior to doing this or you will cause an
 * imbalance in the reference count.
 *
 * @param obj the json_object instance
 * @returns 1 if the object was freed.
 */
int json_object_put(struct json_object *obj);

/**
 * Check if the json_object is of a given type
 * @param obj the json_object instance
 * @param type one of:
     json_type_null (i.e. obj == NULL),
     json_type_boolean,
     json_type_double,
     json_type_int,
     json_type_object,
     json_type_array,
     json_type_string,
 */
extern int json_object_is_type(struct json_object *obj, enum json_type type);

/**
 * Get the type of the json_object.  See also json_type_to_name() to turn this
 * into a string suitable, for instance, for logging.
 *
 * @param obj the json_object instance
 * @returns type being one of:
     json_type_null (i.e. obj == NULL),
     json_type_boolean,
     json_type_double,
     json_type_int,
     json_type_object,
     json_type_array,
     json_type_string,
 */
extern enum json_type json_object_get_type(struct json_object *obj);


/** Stringify object to json format.
 * Equivalent to json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED)
 * @param obj the json_object instance
 * @returns a string in JSON format
 */
extern char* json_object_to_json_string(struct json_object *obj);

/** Stringify object to json format
 * @param obj the json_object instance
 * @param flags formatting options, see JSON_C_TO_STRING_PRETTY and other constants
 * @returns a string in JSON format
 */
extern char* json_object_to_json_string_ext(struct json_object *obj, int flags);

/**
 * Set a custom serialization function to be used when this particular object
 * is converted to a string by json_object_to_json_string.
 *
 * If a custom serializer is already set on this object, any existing 
 * user_delete function is called before the new one is set.
 *
 * If to_string_func is NULL, the other parameters are ignored
 * and the default behaviour is reset.
 *
 * The userdata parameter is optional and may be passed as NULL.  If provided,
 * it is passed to to_string_func as-is.  This parameter may be NULL even
 * if user_delete is non-NULL.
 *
 * The user_delete parameter is optional and may be passed as NULL, even if
 * the userdata parameter is non-NULL.  It will be called just before the
 * json_object is deleted, after it's reference count goes to zero
 * (see json_object_put()).
 * If this is not provided, it is up to the caller to free the userdata at
 * an appropriate time. (i.e. after the json_object is deleted)
 *
 * @param jso the object to customize
 * @param to_string_func the custom serialization function
 * @param userdata an optional opaque cookie
 * @param user_delete an optional function from freeing userdata
 */
extern void json_object_set_serializer(json_object *jso,
    json_object_to_json_string_fn to_string_func,
    void *userdata,
    json_object_delete_fn *user_delete);

/**
 * Simply call free on the userdata pointer.
 * Can be used with json_object_set_serializer().
 *
 * @param jso unused
 * @param userdata the pointer that is passed to free().
 */
json_object_delete_fn json_object_free_userdata;

/**
 * Copy the jso->_userdata string over to pb as-is.
 * Can be used with json_object_set_serializer().
 *
 * @param jso The object whose _userdata is used.
 * @param pb The destination buffer.
 * @param level Ignored.
 * @param flags Ignored.
 */
json_object_to_json_string_fn json_object_userdata_to_json_string;


/* object type methods */

/** Create a new empty object with a reference count of 1.  The caller of
 * this object initially has sole ownership.  Remember, when using
 * json_object_object_add or json_object_array_put_idx, ownership will
 * transfer to the object/array.  Call json_object_get if you want to maintain
 * shared ownership or also add this object as a child of multiple objects or
 * arrays.  Any ownerships you acquired but did not transfer must be released
 * through json_object_put.
 *
 * @returns a json_object of type json_type_object
 */
extern struct json_object* json_object_new_object(void);

/** Get the hashtable of a json_object of type json_type_object
 * @param obj the json_object instance
 * @returns a linkhash
 */
extern struct lh_table* json_object_get_object(struct json_object *obj);

/** Get the size of an object in terms of the number of fields it has.
 * @param obj the json_object whose length to return
 */
extern int json_object_object_length(struct json_object* obj);

/** Add an object field to a json_object of type json_type_object
 *
 * The reference count will *not* be incremented. This is to make adding
 * fields to objects in code more compact. If you want to retain a reference
 * to an added object, independent of the lifetime of obj, you must wrap the
 * passed object with json_object_get.
 *
 * Upon calling this, the ownership of val transfers to obj.  Thus you must
 * make sure that you do in fact have ownership over this object.  For instance,
 * json_object_new_object will give you ownership until you transfer it,
 * whereas json_object_object_get does not.
 *
 * @param obj the json_object instance
 * @param key the object field name (a private copy will be duplicated)
 * @param val a json_object or NULL member to associate with the given field
 */
extern void json_object_object_add(struct json_object* obj, const char *key,
                   struct json_object *val);

/** Get the json_object associate with a given object field
 *
 * *No* reference counts will be changed.  There is no need to manually adjust
 * reference counts through the json_object_put/json_object_get methods unless
 * you need to have the child (value) reference maintain a different lifetime
 * than the owning parent (obj). Ownership of the returned value is retained
 * by obj (do not do json_object_put unless you have done a json_object_get).
 * If you delete the value from obj (json_object_object_del) and wish to access
 * the returned reference afterwards, make sure you have first gotten shared
 * ownership through json_object_get (& don't forget to do a json_object_put
 * or transfer ownership to prevent a memory leak).
 *
 * @param obj the json_object instance
 * @param key the object field name
 * @returns the json_object associated with the given field name
 * @deprecated Please use json_object_object_get_ex
 */
THIS_FUNCTION_IS_DEPRECATED(extern struct json_object* json_object_object_get(struct json_object* obj,
                          const char *key));

/** Get the json_object associated with a given object field.  
 *
 * This returns true if the key is found, false in all other cases (including 
 * if obj isn't a json_type_object).
 *
 * *No* reference counts will be changed.  There is no need to manually adjust
 * reference counts through the json_object_put/json_object_get methods unless
 * you need to have the child (value) reference maintain a different lifetime
 * than the owning parent (obj).  Ownership of value is retained by obj.
 *
 * @param obj the json_object instance
 * @param key the object field name
 * @param value a pointer where to store a reference to the json_object 
 *              associated with the given field name.
 *
 *              It is safe to pass a NULL value.
 * @returns whether or not the key exists
 */
extern json_bool json_object_object_get_ex(struct json_object* obj,
                          const char *key,
                                                  struct json_object **value);

/** Delete the given json_object field
 *
 * The reference count will be decremented for the deleted object.  If there
 * are no more owners of the value represented by this key, then the value is
 * freed.  Otherwise, the reference to the value will remain in memory.
 *
 * @param obj the json_object instance
 * @param key the object field name
 */
extern void json_object_object_del(struct json_object* obj, const char *key);

/**
 * Iterate through all keys and values of an object.
 *
 * Adding keys to the object while iterating is NOT allowed.
 *
 * Deleting an existing key, or replacing an existing key with a
 * new value IS allowed.
 *
 * @param obj the json_object instance
 * @param key the local name for the char* key variable defined in the body
 * @param val the local name for the json_object* object variable defined in
 *            the body
 */
#if defined(__GNUC__) && !defined(__STRICT_ANSI__) && __STDC_VERSION__ >= 199901L

# define json_object_object_foreach(obj,key,val) \
    char *key; \
    struct json_object *val __attribute__((__unused__)); \
    for(struct lh_entry *entry ## key = json_object_get_object(obj)->head, *entry_next ## key = NULL; \
        ({ if(entry ## key) { \
            key = (char*)entry ## key->k; \
            val = (struct json_object*)entry ## key->v; \
            entry_next ## key = entry ## key->next; \
        } ; entry ## key; }); \
        entry ## key = entry_next ## key )

#else /* ANSI C or MSC */

# define json_object_object_foreach(obj,key,val) \
    char *key;\
    struct json_object *val; \
    struct lh_entry *entry ## key; \
    struct lh_entry *entry_next ## key = NULL; \
    for(entry ## key = json_object_get_object(obj)->head; \
        (entry ## key ? ( \
            key = (char*)entry ## key->k, \
            val = (struct json_object*)entry ## key->v, \
            entry_next ## key = entry ## key->next, \
            entry ## key) : 0); \
        entry ## key = entry_next ## key)

#endif /* defined(__GNUC__) && !defined(__STRICT_ANSI__) && __STDC_VERSION__ >= 199901L */

/** Iterate through all keys and values of an object (ANSI C Safe)
 * @param obj the json_object instance
 * @param iter the object iterator
 */
#define json_object_object_foreachC(obj,iter) \
 for(iter.entry = json_object_get_object(obj)->head; (iter.entry ? (iter.key = (char*)iter.entry->k, iter.val = (struct json_object*)iter.entry->v, iter.entry) : 0); iter.entry = iter.entry->next)

/* Array type methods */

/** Create a new empty json_object of type json_type_array
 * @returns a json_object of type json_type_array
 */
extern struct json_object* json_object_new_array(void);

/** Get the arraylist of a json_object of type json_type_array
 * @param obj the json_object instance
 * @returns an arraylist
 */
extern struct array_list* json_object_get_array(struct json_object *obj);

/** Get the length of a json_object of type json_type_array
 * @param obj the json_object instance
 * @returns an int
 */
extern int json_object_array_length(struct json_object *obj);

/** Sorts the elements of jso of type json_type_array
*
* Pointers to the json_object pointers will be passed as the two arguments
* to @sort_fn
*
* @param obj the json_object instance
* @param sort_fn a sorting function
*/
extern void json_object_array_sort(struct json_object *jso, int(__cdecl* sort_fn)(const void *, const void *));

/** Add an element to the end of a json_object of type json_type_array
 *
 * The reference count will *not* be incremented. This is to make adding
 * fields to objects in code more compact. If you want to retain a reference
 * to an added object you must wrap the passed object with json_object_get
 *
 * @param obj the json_object instance
 * @param val the json_object to be added
 */
extern int json_object_array_add(struct json_object *obj,
                 struct json_object *val);

/** Insert or replace an element at a specified index in an array (a json_object of type json_type_array)
 *
 * The reference count will *not* be incremented. This is to make adding
 * fields to objects in code more compact. If you want to retain a reference
 * to an added object you must wrap the passed object with json_object_get
 *
 * The reference count of a replaced object will be decremented.
 *
 * The array size will be automatically be expanded to the size of the
 * index if the index is larger than the current size.
 *
 * @param obj the json_object instance
 * @param idx the index to insert the element at
 * @param val the json_object to be added
 */
extern int json_object_array_put_idx(struct json_object *obj, int idx,
                     struct json_object *val);

/** Get the element at specificed index of the array (a json_object of type json_type_array)
 * @param obj the json_object instance
 * @param idx the index to get the element at
 * @returns the json_object at the specified index (or NULL)
 */
extern struct json_object* json_object_array_get_idx(struct json_object *obj,
                             int idx);

/* json_bool type methods */

/** Create a new empty json_object of type json_type_boolean
 * @param b a json_bool TRUE or FALSE (0 or 1)
 * @returns a json_object of type json_type_boolean
 */
extern struct json_object* json_object_new_boolean(json_bool b);

/** Get the json_bool value of a json_object
 *
 * The type is coerced to a json_bool if the passed object is not a json_bool.
 * integer and double objects will return FALSE if there value is zero
 * or TRUE otherwise. If the passed object is a string it will return
 * TRUE if it has a non zero length. If any other object type is passed
 * TRUE will be returned if the object is not NULL.
 *
 * @param obj the json_object instance
 * @returns a json_bool
 */
extern json_bool json_object_get_boolean(struct json_object *obj);


/* int type methods */

/** Create a new empty json_object of type json_type_int
 * Note that values are stored as 64-bit values internally.
 * To ensure the full range is maintained, use json_object_new_int64 instead.
 * @param i the integer
 * @returns a json_object of type json_type_int
 */
extern struct json_object* json_object_new_int(int32_t i);


/** Create a new empty json_object of type json_type_int
 * @param i the integer
 * @returns a json_object of type json_type_int
 */
extern struct json_object* json_object_new_int64(int64_t i);


/** Get the int value of a json_object
 *
 * The type is coerced to a int if the passed object is not a int.
 * double objects will return their integer conversion. Strings will be
 * parsed as an integer. If no conversion exists then 0 is returned
 * and errno is set to EINVAL. null is equivalent to 0 (no error values set)
 *
 * Note that integers are stored internally as 64-bit values.
 * If the value of too big or too small to fit into 32-bit, INT32_MAX or
 * INT32_MIN are returned, respectively.
 *
 * @param obj the json_object instance
 * @returns an int
 */
extern int32_t json_object_get_int(struct json_object *obj);

/** Get the int value of a json_object
 *
 * The type is coerced to a int64 if the passed object is not a int64.
 * double objects will return their int64 conversion. Strings will be
 * parsed as an int64. If no conversion exists then 0 is returned.
 *
 * NOTE: Set errno to 0 directly before a call to this function to determine
 * whether or not conversion was successful (it does not clear the value for
 * you).
 *
 * @param obj the json_object instance
 * @returns an int64
 */
extern int64_t json_object_get_int64(struct json_object *obj);


/* double type methods */

/** Create a new empty json_object of type json_type_double
 * @param d the double
 * @returns a json_object of type json_type_double
 */
extern struct json_object* json_object_new_double(double d);

/**
 * Create a new json_object of type json_type_double, using
 * the exact serialized representation of the value.
 *
 * This allows for numbers that would otherwise get displayed
 * inefficiently (e.g. 12.3 => "12.300000000000001") to be
 * serialized with the more convenient form.
 *
 * Note: this is used by json_tokener_parse_ex() to allow for
 *   an exact re-serialization of a parsed object.
 *
 * An equivalent sequence of calls is:
 * @code
 *   jso = json_object_new_double(d);
 *   json_object_set_serializer(d, json_object_userdata_to_json_string,
 *       strdup(ds), json_object_free_userdata)
 * @endcode
 *
 * @param d the numeric value of the double.
 * @param ds the string representation of the double.  This will be copied.
 */
extern struct json_object* json_object_new_double_s(double d, const char *ds);

/** Get the double floating point value of a json_object
 *
 * The type is coerced to a double if the passed object is not a double.
 * integer objects will return their double conversion. Strings will be
 * parsed as a double. If no conversion exists then 0.0 is returned and
 * errno is set to EINVAL. null is equivalent to 0 (no error values set)
 *
 * If the value is too big to fit in a double, then the value is set to
 * the closest infinity with errno set to ERANGE. If strings cannot be
 * converted to their double value, then EINVAL is set & NaN is returned.
 *
 * Arrays of length 0 are interpreted as 0 (with no error flags set).
 * Arrays of length 1 are effectively cast to the equivalent object and
 * converted using the above rules.  All other arrays set the error to
 * EINVAL & return NaN.
 *
 * NOTE: Set errno to 0 directly before a call to this function to
 * determine whether or not conversion was successful (it does not clear
 * the value for you).
 *
 * @param obj the json_object instance
 * @returns a double floating point number
 */
extern double json_object_get_double(struct json_object *obj);


/* string type methods */

/** Create a new empty json_object of type json_type_string
 *
 * A copy of the string is made and the memory is managed by the json_object
 *
 * @param s the string
 * @returns a json_object of type json_type_string
 */
extern struct json_object* json_object_new_string(const char *s);

extern struct json_object* json_object_new_string_len(const char *s, size_t len);

/** Get the string value of a json_object
 *
 * If the passed object is not of type json_type_string then the JSON
 * representation of the object is returned.
 *
 * The returned string memory is managed by the json_object and will
 * be freed when the reference count of the json_object drops to zero.
 *
 * @param obj the json_object instance
 * @returns a string
 */
extern char* json_object_get_string(struct json_object *obj);

/** Get the string length of a json_object
 *
 * If the passed object is not of type json_type_string then zero
 * will be returned.
 *
 * @param obj the json_object instance
 * @returns int
 */
extern int json_object_get_string_len(struct json_object *obj);

#ifdef __cplusplus
}
#endif

#endif
