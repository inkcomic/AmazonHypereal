#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #1 $


class ClientWrapper(object):

    def __init__(self, wrapped_client):
        self.__wrapped_client = wrapped_client
        self.__client_type = type(wrapped_client).__name__

    @property
    def verbose(self):
        return self.__verbose

    @verbose.setter
    def verbose(self, value):
        self.__verbose = value

    @property
    def client_type(self):
        return self.__client_type

    def __getattr__(self, attr):
        orig_attr = self.__wrapped_client.__getattribute__(attr)
        if callable(orig_attr):
            def logging_wrapper(*args, **kwargs):
                self.__log_attempt(attr, args, kwargs)
                try:
                    result = orig_attr(*args, **kwargs)
                    self.__log_success(attr, result)
                    return result
                except Exception as e:
                    self.__log_failure(attr, e)
                    raise e
            return logging_wrapper
        else:
            return orig_attr

    def log(self, method_name, log_msg):

        msg = 'AWS '
        msg += self.__client_type
        msg += '.'
        msg += method_name
        msg += ' '
        msg += log_msg

        print msg


    def __log_attempt(self, method_name, args, kwargs):

        msg = 'attempt: '

        comma_needed = False
        for arg in args:
            if comma_needed: msg += ', '
            msg += arg
            comma_needed = True

        for key,value in kwargs.iteritems():
            if comma_needed: msg += ', '
            msg += key
            msg += '='
            if isinstance(value, basestring):
                msg += '"'
                msg += value
                msg += '"'
            else:
                msg += str(value)
            comma_needed = True

        self.log(method_name, msg)


    def __log_success(self, method_name, result):

        msg = 'success: '
        msg += str(result)

        self.log(method_name, msg)


    def __log_failure(self, method_name, e):

        msg = ' failure '
        msg += type(e).__name__
        msg += ': '
        msg += str(getattr(e, 'response', e.message))

        self.log(method_name, msg)
