function Image (element)
  element.src = 'ClassicTileCascadeHelp\\' .. element.src
  return element
end

function Link (element)
	return element.content
end