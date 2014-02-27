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

module CRISP
  module Util
    # Option-data storage/representation.  Used to declare/define a single program
    # option.
    class Option
      private
      @short_name = nil
      @long_name = nil
      @description = nil
      @argument_description = nil
      @argument_optional = false
      @handler_proc = nil

      public

      attr_reader :short_name, :long_name, :description, :argument_description

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
      # @param [block] handler_proc Option handler.  If the option takes an argument,
      #     it will be passed to the block.
      def initialize(short_name, long_name, description,
                     argument_description = nil, argument_optional = false,
                     &handler_proc)
        raise 'Short name and long name cannot both be `nil\'!' if
          short_name.nil? and long_name.nil?

        @short_name = short_name
        @long_name = long_name

        @handler_proc = handler_proc

        @description = description
        @argument_description = argument_description
        @argument_optional = argument_optional
      end

      def received(argument = nil)
        @handler_proc.call(argument)
      end

      def takes_argument?
        not @argument_description.nil?
      end

      def argument_optional?
        @argument_optional
      end

      def to_string(max_len)
        out = '  '
        
        out += '-' + @short_name if not @short_name.nil?
        out += ', ' if not short_name.nil? and not @long_name.nil?
        out += '--' + @long_name if not @long_name.nil?

        if not @argument_description.nil?
          if not @argument_optional
            out += ' ' + @argument_description
          else
            out += '[=' + @argument_description + ']'
          end
        end

        sprintf('%%-%ds%%s' % (max_len + 2) %
                [out, Option.wrap_text(@description, max_len + 2)])
      end

      def self.wrap_text(s, start_col=0, max_col=75)
        width = max_col - start_col
        if s.length <= width
          return s
        else
          out = ''
          while s.length > width
            front = s[0...width]
            parts = front.rpartition(/\s+/)
            s = parts[2].strip + s[width..-1]
            out += "%s\n%s" % [parts[0], ' ' * start_col]
          end
          out += s.strip
          out
        end
      end


      def <=>(otherOption)
        if not @long_name.nil? and not otherOption.long_name.nil?
          @long_name.upcase <=> otherOption.long_name.upcase
        elsif not @short_name.nil? and not otherOption.short_name.nil?
          @short_name.upcase <=> otherOption.short_name.upcase
          # elsif not @long_name.nil?
          #   @long_name.upcase <=> otherOption.short_name.upcase
          # else
          #   @short_name.upcase <=> otherOption.long_name.upcase
        end
      end

      def getPreferredKey()
        if not @long_name.nil? then @long_name else @short_name end
      end
    end

    # Stateless option-parser functions.
    module OptionParser
      # Handle script arguments according to the given options array.
      def OptionParser.handle_options(option_list, arguments = ARGV)
        option_list = option_list.values.flatten() if option_list.kind_of?(Hash)
        short_option_map = { }
        long_option_map = { }
        option_list.each { |o|
          short_option_map[o.short_name] = o
          long_option_map[o.long_name] = o
        }
        i = 0
        while ( i < arguments.length ) do
          n = check_option(short_option_map, long_option_map, arguments[i], arguments[i+1])
          if n === false
            break
          elsif n == 0
            i += 1
          else
            (i...(i+n)).each { |j| arguments.delete_at(i) }
          end
        end
      end  

      # Print a usage string, with alignment adjustments for multi-usage programs,
      # to an arbitrary stream.
      def OptionParser.show_usage(usage, io = $stdout)
        # Fix alignment for multi-line usage strings.
        usage = usage.split("\n").join("\n       ") if usage.include?("\n")
        io.print("Usage: %s\n" % usage)
      end    


      # Print a formatted help message (as is often triggered via a program's
      # `--help` command-line option) to an arbitrary stream.
      def OptionParser.show_help(option_list, usage, description, io = $stdout)
        all_options = option_list
        all_options = option_list.values.flatten() if option_list.kind_of?(Hash)

        max_len = 0
        all_options.each { |o|
          optarg_length = 2     # two spaces for the margin

          optarg_length += 2 if not o.short_name.nil? and not o.long_name.nil?
          optarg_length += 2 + o.long_name.length if not o.long_name.nil?
          optarg_length += 1 + o.short_name.length if not o.short_name.nil?

          if not o.argument_description.nil?
            optarg_length += 1 + o.argument_description.length
            optarg_length += 3 if o.argument_optional?
          end
          max_len = optarg_length if optarg_length > max_len
        }

        self.show_usage(usage, io)
        io.print( (if description.nil? then '' else "\n%s\n" % description end))
        if option_list.kind_of?(Array)
          io.print("\nOptions:\n")

          option_list.each { |o| io.print o.to_string(max_len) + "\n" }
        else
          option_list.each_pair { |k, v|
            io.print("\n#{k}:\n")
            v.each { |o| io.print o.to_string(max_len) + "\n" }
          }
        end
      end

      def OptionParser.handle_long_option(long_option_map, arg, next_arg)
        return 0 if arg[0].chr != '-' or arg[1].chr != '-'
        equalsIndex = arg.index('=')
        equalsIndex = 0 if equalsIndex.nil?
        optionName = arg[2..(equalsIndex-1)];
        raise 'No such option, `--%s\'!' if not long_option_map.has_key?(optionName)
        option = long_option_map[optionName]

        if option.takes_argument?
          if option.argument_optional?
            if equalsIndex == 0
              $stderr.print('Warning: Please use `--long-option=value\' syntax for optional arguments.'+"\n") if not next_arg.nil?
              option.received()
              1
            else
              option.received(arg[(equalsIndex + 1)..-1])
              1
            end
          else                    # option.argument_optional?
            if equalsIndex == 0
              raise 'Missing required argument to `--' + optionName + '\'!' if next_arg.nil?
              option.received(next_arg)
              2
            else
              option.received(arg[(equalsIndex + 1)..-1])
              1
            end
          end                     # option.argument_optional?
        else                      # option.takes_argument?
          option.received()
          1
        end                           # option.takes_argument?
      end

      def OptionParser.handle_short_options(short_option_map, arg, next_arg)
        return 0 if arg[0].chr != '-'
        consumed = 1
        i = 1
        while ( i < arg.length )
          raise 'No such option, `-%s\'!' % arg[i].chr if not short_option_map.has_key?(arg[i].chr)
          option = short_option_map[arg[i].chr]
          n = 0
          if option.takes_argument?
            if option.argument_optional?
              $stderr.print('Warning: Please use `--long-option=VALUE\' syntax when specifying optional arguments.'+"\n") if not next_arg.nil?
              option.received()
              n = 1
            else
              if i < arg.length - 1
                option.received(arg[(i+1)..-1])
                i = arg.length
                n = 1
              else
                raise 'Missing required argument to `-' + arg[i].chr + '\'!' if next_arg.nil?
                option.received(next_arg)
                i = arg.length
                n = 2
              end
            end
          else
            option.received()
            n = 1
          end
          consumed = [n, consumed].max
          i += 1
        end
        consumed
      end 


      def OptionParser.check_option(short_option_map, long_option_map, arg, next_arg)
        if not arg.nil? and arg.length > 0 and arg[0].chr == '-'
          if arg.length > 1
            if arg[1].chr == '-'
              return arg.length == 2 ? false : handle_long_option(long_option_map, arg, next_arg)
            else
              return handle_short_options(short_option_map, arg, next_arg)
            end
          else
            0
          end                         # arg[1] == '-'
        else
          0
        end
      end
    end
  end
end
