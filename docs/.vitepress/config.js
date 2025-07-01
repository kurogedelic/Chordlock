export default {
  title: 'Chordlock',
  description: 'Advanced Key-Context Chord Detection API',
  base: '/Chordlock/',
  
  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'API Reference', link: '/api/' },
      { text: 'Examples', link: '/examples' },
      { text: 'GitHub', link: 'https://github.com/kurogedelic/Chordlock' }
    ],

    sidebar: [
      {
        text: 'Getting Started',
        items: [
          { text: 'Introduction', link: '/' },
          { text: 'Quick Start', link: '/quick-start' },
          { text: 'Installation', link: '/installation' }
        ]
      },
      {
        text: 'API Reference',
        items: [
          { text: 'CLI Interface', link: '/api/cli' },
          { text: 'C++ Library', link: '/api/cpp' },
          { text: 'WebAssembly', link: '/api/wasm' },
          { text: 'MCP Integration', link: '/api/mcp' }
        ]
      },
      {
        text: 'Advanced Features',
        items: [
          { text: 'Key Context Analysis', link: '/features/key-context' },
          { text: 'Roman Numeral Analysis', link: '/features/roman-numerals' },
          { text: 'Functional Harmony', link: '/features/functional-harmony' }
        ]
      },
      {
        text: 'Examples',
        items: [
          { text: 'Basic Usage', link: '/examples/basic' },
          { text: 'Web Integration', link: '/examples/web' },
          { text: 'Advanced Scenarios', link: '/examples/advanced' }
        ]
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/kurogedelic/Chordlock' }
    ],

    footer: {
      message: 'Released under the MIT License.',
      copyright: 'Copyright © 2024-2025 Leo Kuroshita'
    }
  }
}