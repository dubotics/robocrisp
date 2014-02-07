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

# Issues:
#
#   * This code was written back when I was still experimenting with different
#     coding styles (such as Java style in Ruby, yes...).  Needs
#     de-Javafication.  (I've de-javafied the names of a couple commonly-used
#     methods in OptionParser.)
#
#   * The file is mostly undocumented.  (I've added some comments on the
#     important parts.)
#
#   * Option-argument candidate values, as well as default option-argument
#     values, are not integrated into the parser code.  They were introduced to
#     support builtin tab-completion for BASh in one of my scripts, but still
#     need better support in `OptionParser`.

module CRISP
  module Util
    # Option-data storage/representation.  Used to declare/define a single program
    # option.
    class Option
      private
      @shortName = nil
      @longName = nil
      @description = nil
      @argumentDescription = nil
      @argumentOptional = false
      @argumentCandidates = nil
      @defaultArgument = nil

      @handlerProc = nil

      public

      attr_reader :shortName, :longName, :description, :argumentDescription
      attr_accessor :argumentCandidates

      # Initialize a new Option instance.
      #
      # @param [String] shortName Single-character value to identify the option's
      #     short form (single-dash prefix).
      #
      # @param [String] longName Long-form (double-dash style) name for the option.
      #
      # @param [String] description Description of what the option does.
      #
      # @param [String,nil] argumentDescription If non-`nil`, a single-word
      #     (upper-case preferred) that suggests the kind of argument the option
      #     takes.
      #
      # @param [Boolean] argumentOptional Whether the option's argument is optional;
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
      def initialize(shortName, longName, description,
                     argumentDescription = nil, argumentOptional = false, argumentCandidates = nil,
                     defaultArgument = nil, &handlerProc)
        raise 'Short name and long name cannot both be `nil\'!' if shortName.nil? and longName.nil?

        @shortName = shortName
        @longName = longName

        @handlerProc = handlerProc

        @description = description
        @argumentDescription = argumentDescription
        @argumentOptional = argumentOptional
        @argumentCandidates = argumentCandidates
        @defaultArgument = defaultArgument
      end

      def received(argument = nil)
        @handlerProc.call(argument)
      end

      def takesArgument?
        if not @argumentDescription.nil? then true else false end
      end

      def argumentOptional?
        @argumentOptional
      end

      def toString(maxLongNameLength, maxOptArgNameLength)
        out = '  '
        
        out += '-' + @shortName if not @shortName.nil?
        out += ', ' if not shortName.nil? and not @longName.nil?

        s = ''
        if not @argumentDescription.nil?
          s += ' ' if not @argumentOptional
          s += '[=' if @argumentOptional
          s += @argumentDescription
          s += ']' if @argumentOptional
          s += ' '
        end

        out += sprintf('%s%-' + (maxLongNameLength + maxOptArgNameLength + 2).to_s() + 's',
                       (if @longName.nil? then '  ' else '--' end),
                       (if longName.nil? then '' else @longName end) + s ) + @description
      end

      def <=>(otherOption)
        if not @longName.nil? and not otherOption.longName.nil?
          @longName.upcase <=> otherOption.longName.upcase
        elsif not @shortName.nil? and not otherOption.shortName.nil?
          @shortName.upcase <=> otherOption.shortName.upcase
          # elsif not @longName.nil?
          #   @longName.upcase <=> otherOption.shortName.upcase
          # else
          #   @shortName.upcase <=> otherOption.longName.upcase
        end
      end

      def getPreferredKey()
        if not @longName.nil? then @longName else @shortName end
      end
    end

    # Stateless option-parser functions.
    module OptionParser
      # Handle script arguments according to the given options array.
      def OptionParser.handle_options(optionList, arguments = ARGV)
        shortOptionMap = { }
        longOptionMap = { }
        optionList.each { |o|
          shortOptionMap[o.shortName] = o
          longOptionMap[o.longName] = o
        }
        i = 0
        while ( i < arguments.length ) do
          n = checkOption(shortOptionMap, longOptionMap, arguments[i], arguments[i+1])
          ( arguments.delete_at(0); break ) if n === false
          if n == 0
            i += 1
          else
            (0...n).each { arguments.delete_at(i) }
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
      def OptionParser.show_help(optionList, usage, description, io = $stdout)
        maxLongNameLength = 0
        maxOptArgNameLength = 0
        optionList.each { |o|
          longNameLength = 0
          optArgNameLength = 0

          longNameLength = o.longName.length if not o.longName.nil?
          if not o.argumentDescription.nil?
            optArgNameLength = o.argumentDescription.length
            optArgNameLength += 2 if o.argumentOptional?
          end

          maxLongNameLength = longNameLength if longNameLength > maxLongNameLength
          maxOptArgNameLength = optArgNameLength if optArgNameLength > maxOptArgNameLength
        }

        self.show_usage(usage, io)
        io.print( (if description.nil? then '' else "\n%s\n" % description end) + "\n")
        io.print("Options:\n")

        optionList.each { |o| io.print o.toString(maxLongNameLength, maxOptArgNameLength) + "\n" }
      end

      def OptionParser.handleLongOption(longOptionMap, arg, nextArg)
        equalsIndex = arg.index('=')
        equalsIndex = 0 if equalsIndex.nil?
        optionName = arg[2 .. equalsIndex - 1];

        option = longOptionMap[optionName];
        if option.is_a?(Option)
          if option.takesArgument?
            if option.argumentOptional?
              if equalsIndex == 0
                $stderr.print('Warning: Please use `--long-option=value\' syntax for optional arguments.'+"\n") if not nextArg.nil?
                option.received()
                1
              else
                option.received(arg[equalsIndex + 1..-1])
                1
              end
            else                    # option.argumentOptional?
              if equalsIndex == 0
                raise 'Missing required argument to `--' + optionName + '\'!' if nextArg.nil?
                option.received(nextArg)
                2
              else
                option.received(arg[equalsIndex + 1..-1])
                1
              end
            end                     # option.argumentOptional?
          else                      # option.takesArgument?
            option.received()
            1
          end                           # option.takesArgument?
        else
          raise 'No such option, `--' + optionName + '\'!'
        end
      end

      def OptionParser.handleShortOptions(shortOptionMap, arg, nextArg)
        consumed = 0
        i = 1
        while ( i < arg.length ) do
          option = shortOptionMap[arg[i].chr]
          if option.is_a?(Option)
            n = (if option.takesArgument?
                   if option.argumentOptional?
                     $stderr.print('Warning: Please use `--long-option=VALUE\' syntax when specifying optional arguments.'+"\n") if not nextArg.nil?
                     option.received()
                     1
                   else              # option.argumentOptional?
                     if i < arg.length - 1
                       option.received(arg[i+1..-1])
                       i = arg.length
                       1
                     else
                       raise 'Missing required argument to `-' + arg[i].chr + '\'!' if nextArg.nil?
                       option.received(nextArg)
                       2
                     end
                   end                     # option.argumentOptional?
                 else                      # option.takesArgument?
                   option.received(nil)
                   1
                 end)                     # option.takesArgument?
          else
            raise 'No such option, `-' + arg[i].chr + '\'!'
          end
          consumed = n if n > consumed
          i += 1
        end                         # while ( i < arg.length )
        consumed
      end 


      def OptionParser.checkOption(shortOptionMap, longOptionMap, arg, nextArg)
        if not arg.nil? and arg.length > 0 and arg[0].chr == '-'
          if arg.length > 1
            if arg[1].chr == '-'
              return arg.length == 2 ? false : handleLongOption(longOptionMap, arg, nextArg)
            else
              return handleShortOptions(shortOptionMap, arg, nextArg)
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
