const struct http_parser_settings Parser::settings = {
  Proxy<Call, &Parser::on_message_begin>::Raw,
  Proxy<DataCall, &Parser::on_url>::Raw,
  Proxy<DataCall, &Parser::on_status>::Raw,
  Proxy<DataCall, &Parser::on_header_field>::Raw,
  Proxy<DataCall, &Parser::on_header_value>::Raw,
  Proxy<Call, &Parser::on_headers_complete>::Raw,
  Proxy<DataCall, &Parser::on_body>::Raw,
  Proxy<Call, &Parser::on_message_complete>::Raw,
  nullptr,
  nullptr
};