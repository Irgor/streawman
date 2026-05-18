# 016 — Frontend Styling Approach

**Date**: 2026-05-18
**Status**: Accepted
**Origin**: Developer identified Tailwind as the obvious choice. AI confirmed and noted the alignment with the existing system design token plan.

## Context

`streawman-ui` (Next.js) needs a styling approach that is consistent, maintainable, and demonstrates modern frontend skills.

## Options Considered

**Tailwind CSS**
- Utility-first CSS framework; styles written directly in JSX as class names
- First-class Next.js support — offered by `create-next-app` out of the box
- Built-in design token system (spacing, color, typography scales via `tailwind.config`)
- No context switching between component and style files
- Industry standard for React/Next.js projects in 2025–2026
- CV: expected by the market for modern frontend roles

**CSS Modules**
- Scoped CSS files per component (`Button.module.css`)
- True style isolation — ideal for large shared component libraries
- More file overhead; context switching between `.tsx` and `.module.css`
- Better fit for design-system packages distributed as npm packages; overkill for a single app

**styled-components / Emotion**
- CSS-in-JS; styles as tagged template literals in component files
- Poor Server Component compatibility in Next.js App Router — runtime CSS-in-JS requires client-side hydration
- Eliminated by the SSR-first requirement

## Decision

**Tailwind CSS**

The combination of Next.js native support, built-in token system, and zero context switching makes Tailwind the straightforward choice. The `tailwind.config.js` file becomes the source of truth for design tokens (colors, spacing, typography) — directly replacing the `design-system/tokens/` markdown docs with executable config.

## Tailwind Config as Design Token Source

Rather than maintaining separate token markdown files, define tokens in `tailwind.config.js`:

```js
// tailwind.config.js — this IS the design system token file
module.exports = {
  theme: {
    extend: {
      colors: {
        // [FILL: define brand colors here before writing any UI]
        primary: { ... },
        surface: { ... },
      },
      fontFamily: {
        // [FILL: define font stacks]
      },
    },
  },
}
```

## Rules

- No arbitrary values (`w-[347px]`) unless no token fits — prefer extending the config
- No inline `style={}` except for values computed at runtime that cannot be a class
- Dark mode: [FILL: decide before starting UI — `class` strategy or `media` strategy]

## Consequences

- `tailwind.config.js` replaces `design-system/tokens/*.md` as the token source of truth
- `create-next-app` installs Tailwind automatically when selected during scaffolding
- PostCSS config is generated automatically alongside Tailwind
