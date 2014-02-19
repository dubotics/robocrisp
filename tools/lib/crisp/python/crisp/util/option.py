# Custom option and option-parser classes with emphasis on natural, easy-to-use
# syntax for option declaration and parsing.
#
# Copyright (C) 2009-2014 Collin J. Sutton.  All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details. 
#
# The GNU General Public License version 2 may be found at
# <http://www.gnu.org/licenses/gpl-2.0.html>.

import sys

# Option-data storage/representation.  Used to declare/define a single program
# option.
class Option:
    short_name = None
    long_name = None
    description = None
    argument_description = None
    argument_optional = None
    callback = None
    
    # Initialize a new Option instance.
    #
    # @param [String] short_name Single-character value to identify the option's
    #     short form (single-dash prefix).
    #
    # @param [String] long_name Long-form (double-dash style) name for the option.
    #
    # @param [String] description Description of what the option does.
    #
    # @param [String,nil] argument_description If non-`nil`, a single-word
    #     (upper-case preferred) that suggests the kind of argument the option
    #     takes.
    #
    # @param [Boolean] argument_optional Whether the option's argument is optional;
    #     only meaningful if an argument description was given.
    #
    # @param [Array<String>,nil] argumentCandidates An array of accepted values
    #     for the argument, or `nil`.
    #
    # @param [String,nil] defaultArgument Default value for the argument; only
    #     meaningful if an argument description was given and the argument was
    #     specified as optional.
    #
    # @param [block] handlerProc Option handler.  If the option takes an argument,
    #     it will be passed to the block.
    def __init__(self, short_name, long_name, description,
                 argument_description = None, argument_optional = False):
        if short_name == None and long_name == None:
            raise ArgumentError.new('Short name and long name cannot both be `nil\'!')
        self.short_name = short_name
        self.long_name = long_name
        self.description = description
        self.argument_description = argument_description
        self.argument_optional = argument_optional
        self.callback = None

    def handle(self, callback):
        self.callback = callback
        return self

    def received(self, *args):
        if isinstance(self.callback, str):
            lcls = globals()
            if len(args) == 0:
                lcls[self.callback] = True
            elif len(args) == 1:
                lcls[self.callback] = args[0]
        elif callable(self.callback):
                self.callback(*args)


    def takes_argument(self):
        return self.argument_description != None


    def to_string(self, maxLongNameLength, maxOptArgNameLength):
        out = '  '
        
        if self.short_name != None:
            out += '-' + self.short_name 
        if self.short_name != None and self.long_name != None:
            out += ', '

        s = ''
        if self.argument_description != None:
            if not self.argument_optional:
                s += ' ' 
            else:
                s += '[='
            s += self.argument_description
            if self.argument_optional:
                s += ']'
            s += ' '

        foo = None
        bar = None
        if self.long_name == None:
            foo = '  '
            bar = ''
        else:
            foo = '--'
            bar = self.long_name

        return out + ('%%s%%-%ds%%s' % (maxLongNameLength + maxOptArgNameLength + 2)) % (foo, bar + s, self.description)

      # def <=>(otherOption)
      #   if not self.long_name == None and not otherOption.long_name == None
      #     self.long_name.upcase <=> otherOption.long_name.upcase
      #   elsif not self.short_name == None and not otherOption.short_name == None
      #     self.short_name.upcase <=> otherOption.short_name.upcase
      #     # elsif not self.long_name == None
      #     #   self.long_name.upcase <=> otherOption.short_name.upcase
      #     # else
      #     #   self.short_name.upcase <=> otherOption.long_name.upcase
      #   end
      # end

    def get_preferred_key(self):
        if self.long_name != None:
            return self.long_name
        else:
            return self.short_name

class OptionParser:
    """Stateless option-parser functions."""

    @classmethod
    def handle_options(self, option_list, arguments = sys.argv[1:]):
        """Handle script arguments according to the given options array."""
        short_option_map = { }
        long_option_map = { }
        for o in option_list:
            short_option_map[o.short_name] = o
            long_option_map[o.long_name] = o
        i = 0
        while ( i < len(arguments) ):
            next_arg = None
            if i + 1 < len(arguments):
                next_arg = arguments[i+1]
            n = OptionParser.check_option(short_option_map, long_option_map, arguments[i], next_arg)
            if type(n) == bool and n == False:
                break
            elif n == 0:
                break
            else:
                j = i
                while j < i + n:
                    del arguments[j]
                    j += 1


    @classmethod
    def show_usage(self, usage, io = sys.stdout):
        """
        Print a usage string, with alignment adjustments for multi-usage programs, to an
        arbitrary stream.

        """

        # Fix alignment for multi-line usage strings.
        if usage.find("\n") >= 0:
            usage = "\n       ".join(usage.split("\n"))
        io.write("Usage: %s\n" % usage)


    @classmethod
    def show_help(self, option_list, usage, description, io = sys.stdout):
        """
        Print a formatted help message (as is often triggered via a program's `--help`
        command-line option) to an arbitrary stream.

        """
        maxLongNameLength = 0
        maxOptArgNameLength = 0
        for o in option_list:
            long_name_length = 0
            optArgNameLength = 0

            if o.long_name != None:
                long_name_length = len(o.long_name) 
            if not o.argument_description == None:
                optArgNameLength = len(o.argument_description)
                if o.argument_optional():
                    optArgNameLength += 2

            if long_name_length > maxLongNameLength:
                maxLongNameLength = long_name_length
            if optArgNameLength > maxOptArgNameLength:
                maxOptArgNameLength = optArgNameLength

        OptionParser.show_usage(usage, io)
        if description != None:
            io.write("\n%s\n" % description)
            io.write("\nOptions:\n")

        for o in option_list:
            io.write("%s\n" % o.to_string(maxLongNameLength, maxOptArgNameLength))

    @classmethod
    def handle_long_option(self, long_option_map, arg, next_arg):
        equals_index = arg.find('=')

        option_name = None
        if equals_index == -1:
            option_name = arg[2:]
        else:
            option_name = arg[2:equals_index]

        option = long_option_map[option_name];
        if isinstance(option, Option):
            if option.takes_argument():
                if option.argument_optional():
                    if equals_index == 0:
                        if not next_arg == None:
                            sys.stderr.write('Warning: Please use `--long-option=value\' syntax for optional arguments.'+"\n")
                        option.received()
                        return 1
                    else:
                        option.received(arg[(equals_index + 1):])
                        return 1
                else:                    # option.argument_optional?
                    if equals_index == 0:
                        if next_arg == None:
                            raise 'Missing required argument to `--' + option_name + '\'!' 
                        option.received(next_arg)
                        return 2
                    else:
                        option.received(arg[(equals_index + 1):])
                        return 1

            else:                      # option.takesArgument?
                option.received()
                return 1
        else:
            raise 'No such option, `--' + option_name + '\'!'

    @classmethod
    def handle_short_options(self, short_option_map, arg, next_arg):
        consumed = 0
        i = 1
        while ( i < len(arg) ):
            option = short_option_map[arg[i]]
            if isinstance(option, Option):
                n = None
                if option.takes_argument():
                    if option.argument_optional():
                        if next_arg != None:
                            sys.stderr.write('Warning: Please use `--long-option=VALUE\' syntax when specifying optional arguments.'+"\n")
                        option.received()
                        n = 1
                    else:
                        if i < len(arg) - 1:
                            option.received(arg[(i+1):])
                            i = arg.length
                            n = 1
                        else:
                            if next_arg == None:
                                raise 'Missing required argument to `-' + arg[i] + '\'!'
                            option.received(next_arg)
                            n = 2
                else:
                    option.received()
                    n = 1
            else:
                raise 'No such option, `-' + arg[i] + '\'!'

            if n > consumed:
                consumed = n
            i += 1
        return consumed


    @classmethod
    def check_option(self, short_option_map, long_option_map, arg, next_arg):
        if arg != None and len(arg) > 0 and arg[0] == '-':
            if len(arg) > 1:
                if arg[1] == '-':
                    if len(arg) == 2:
                        # This case handles the special argument '--', which by convention means
                        # "don't parse anything after this as an option".
                        return False
                    else:
                        return OptionParser.handle_long_option(long_option_map, arg, next_arg)
                else:
                    return OptionParser.handle_short_options(short_option_map, arg, next_arg)
            else:
                return 0
        else:
            return 0
