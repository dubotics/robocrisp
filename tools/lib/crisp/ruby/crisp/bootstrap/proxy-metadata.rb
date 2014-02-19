# Here we tell the Proxy library about the attributes of specific function 
require 'proxy'

['Program', 'Configuration', 'ProgramInvocationInfo'].each { |f| require_relative f }

Proxy::ObjectNode.set_method_attributes(CRISP::Bootstrap::Program, :modes, [:nocopy])
Proxy::ObjectNode.set_method_attributes(CRISP::Bootstrap::Configuration, :programs, [:nocopy])
Proxy::ObjectNode.set_method_attributes(CRISP::Bootstrap::ProgramInvocationInfo, :exec!, [ :noreturn ])
