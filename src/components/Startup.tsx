import {StartupScreen} from '@stardrop/ui';
import type {ReactNode} from 'react';
export default function Startup({children}:{children:ReactNode}){return <StartupScreen brand="STARLAB" logo="/brand/starlab.png">{children}</StartupScreen>}
